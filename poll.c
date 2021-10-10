/*
 * ISC License
 *
 * Copyright (c) 2021, Tommi Leino <namhas@gmail.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "evsrc.h"
#include "event.h"

#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#include <stdlib.h>
#include <assert.h>

#define FD_CHUNK	1024

struct event {
	struct pollfd	*pfd;
	struct evsrc	**evsrc;
	int		*remaining;
	int		 n;
	int		 alloc;
	int		 lowest_timeout;
};

static int		 lowest_timeout(struct event *);
static void		 dispatch_timers();

static struct event	*_ev;

struct event *
event_create()
{
	struct event *ev;

	/*
	 * Unfortunately it is still a portability mess regarding timers
	 * and event handling. For making it working in most platforms,
	 * we rely on alarm(3) i.e. we rely on signals, and as we rely
	 * on signals we have to rely on globals. For this reason there
	 * can be only one instance of the event object. For a better
	 * implementation, see kqueue.c or implement conditional compiling
	 * for timerfd_create(2) that works in Linux.
	 */
	if (_ev != NULL)
		assert(0);

	ev = calloc(1, sizeof(struct event));
	if (ev == NULL)
		return NULL;

	ev->pfd = calloc(ev->alloc, sizeof(struct pollfd));
	if (ev->pfd == NULL) {
		event_free(ev);
		return NULL;
	}

	ev->evsrc = calloc(ev->alloc, sizeof(struct evsrc *));
	if (ev->evsrc == NULL) {
		event_free(ev);
		return NULL;
	}

	_ev = ev;
	signal(SIGALRM, dispatch_timers);

	return ev;
}

static void
dispatch_timers()
{
	int i;
	struct evsrc *evsrc;
	struct event *ev = _ev;

	assert(_ev != NULL);

	for (i = 0; i < ev->n; i++) {
		evsrc = ev->evsrc[i];
		if (evsrc->type == EVSRC_TIMER) {
			if (evsrc->remaining > 0)
				evsrc->remaining--;
			if (evsrc->remaining == 0) {
				evsrc->readcb(evsrc, evsrc->data);
				evsrc->remaining = evsrc->value / 1000;
			}
		}
	}

	ev->lowest_timeout = lowest_timeout(ev);
	if (ev->lowest_timeout > 0)
		alarm(ev->lowest_timeout);
	else
		alarm(0);
}

void
event_free(struct event *ev)
{
	free(ev->pfd);
	free(ev->evsrc);
	free(ev);
}

static int
lowest_timeout(struct event *ev)
{
	int i;
	int lowest = INT_MAX;
	struct evsrc *evsrc;

	for (i = 0; i < ev->n; i++) {
		evsrc = ev->evsrc[i];
		if (evsrc->type == EVSRC_TIMER && evsrc->remaining < lowest)
			lowest = evsrc->remaining;
	}

	return lowest;
}

int
event_add_evsrc(struct event *ev, struct evsrc *evsrc)
{
	struct pollfd *pfd;
	struct evsrc *src;

	evsrc->ev = ev;

	if (ev->n == ev->alloc) {
		ev->alloc *= 2;
		if (ev->alloc == 0)
			ev->alloc = FD_CHUNK;

		ev->pfd = realloc(ev->pfd,
		    ev->alloc * sizeof(struct pollfd));
		if (ev->pfd == NULL)
			return -1;

		ev->evsrc = realloc(ev->evsrc,
		    ev->alloc * sizeof(struct evsrc *));
		if (ev->evsrc == NULL)
			return -1;
	}

	ev->evsrc[ev->n] = evsrc;
	pfd = &ev->pfd[ev->n];
	ev->n++;

	switch (evsrc->type) {
	case EVSRC_FD:
		pfd->fd = evsrc->value;
		pfd->events = POLLIN;
		break;
	case EVSRC_TIMER:
		pfd->fd = 0;
		pfd->events = 0;
		evsrc->remaining = evsrc->value / 1000;
		if (evsrc->remaining <= 0)
			evsrc->remaining = 1;
		ev->lowest_timeout = lowest_timeout(ev);
		if (ev->lowest_timeout > 0)
			alarm(ev->lowest_timeout);
		break;
	default:
		assert(0);
		break;
	}

	return 0;
}

int
event_dispatch(struct event *ev)
{
	int nready;	
	int i, nevents;
	struct evsrc *evsrc;
	struct pollfd *pfd;

#ifndef INFTIM
#define INFTIM -1
#endif
	nready = poll(ev->pfd, ev->n, INFTIM);
	if (nready == -1 && errno == EINTR)
		return 0;
	else if (nready == -1)
		return -1;

	for (i = 0; i < nready; i++) {
		pfd = &ev->pfd[i];
		evsrc = ev->evsrc[i];

		if (pfd->revents & (POLLIN | POLLHUP)) {
			evsrc->readcb(evsrc, evsrc->data);
			break;
		}
	}

	return 0;
}

#ifdef TEST
int
readcb(struct evsrc *evsrc, void *data)
{
	printf("Got event\n");
	sleep(1);
	return 0;
}

int
timercb(struct evsrc *evsrc, void *data)
{
	printf("Got timer event\n");
	return 0;
}

int
main(int argc, char *argv[])
{
	struct event *ev;
	struct evsrc *fdsrc, *timersrc;

	ev = event_create();
	if (ev == NULL)
		err(1, "event_create");

	fdsrc = evsrc_create_fd(0, readcb, NULL);
	if (fdsrc == NULL)
		err(1, "evsrc_create");

	timersrc = evsrc_create_timer(1000, timercb, NULL);
	if (timersrc == NULL)
		err(1, "evsrc_create");

	if (event_add_evsrc(ev, fdsrc) != 0)
		err(1, "event_add_evsrc");

	if (event_add_evsrc(ev, timersrc) != 0)
		err(1, "event_add_evsrc");

	for (;;)
		if (event_dispatch(ev) == -1)
			err(1, "event_dispatch");
	
	event_free(ev);
}
#endif
