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

#include "object.h"
#include "event.h"
#include "evsrc.h"
#include "room.h"
#include "message.h"
#include "match.h"
#include "player.h"
#include "command.h"
#include "util.h"

#include <err.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>

#include <stdarg.h>

#include <fcntl.h>

#include <errno.h>

#include <string.h>

#include <ctype.h>
#include <assert.h>
#include <inttypes.h>

extern size_t		 parseline(char *, char *, size_t);
extern int		 tcpbind(const char *, int);

static int client_read(struct evsrc *src, void *data);

const char *
file_to_buffer(const char *file)
{
	static char buf[8192];
	size_t n;

	FILE *fp;

	fp = fopen(file, "r");
	if (fp == NULL)
		err(1, "fopen");

	n = fread(buf, sizeof(char), sizeof(buf), fp);
	if (n <= 0) {
		err(1, "fopen");
	}
	buf[n] = '\0';

	fclose(fp);

	return buf;
}
	
int
server_accept(struct evsrc *src, void *data)
{
	int				 fd;
	struct sockaddr_storage		 addr;
	socklen_t			 len = sizeof(addr);
	struct evsrc			*plrsrc;
	struct player			*plr;

	fd = accept(src->value, (struct sockaddr *) &addr, &len);
	if (fd < 0) {
		warn("accept");
		return 0;
	}

	plr = player_create();
	if (plr == NULL) {
		warn("player_create");
		return -1;
	}

	plrsrc = evsrc_create_fd(fd, client_read, plr);
	if (plrsrc == NULL) {
		warn("evsrc_create_fd");
		return -1;
	}
	plr->evsrc = plrsrc;
	event_add_evsrc(src->ev, plrsrc);

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		warn("couldn't set nonblocking");

	tellp(plr, file_to_buffer("welcome"));
	end_fmtbuf(&plr->fmtbuf);

	return 0;
}

static int
client_read(struct evsrc *src, void *data)
{
	struct player *plr = (struct player *) data;

	printf("Got event\n");

	int n;
	int len;
	char dst[READ_BLOCK];

	if (sizeof(plr->buf) - 1 - plr->sz <= 0) {
		warnx("discarded %d bytes; too long line", plr->sz);
		plr->sz = 0;
	}
	n = read(src->value, &plr->buf[plr->sz],
	    sizeof(plr->buf) - 1 - plr->sz);
	if (n > 0) {
		plr->sz += n;
		plr->buf[plr->sz] = '\0';
		while ((len = parseline(plr->buf, dst, sizeof(dst)))
		    != -1) {
			player_input(plr, dst);
			plr->sz -= len;
		}
	} else if (n < 0 || n == 0) {
		object_free(OBJ(plr));
		return -1;
	}

	return 0;
}

int
timercb(struct evsrc *evsrc, void *data)
{
	return 0;
}

static void
load_world(const char *file)
{
	FILE				*fp;
	struct object			*obj, *env;
	struct player			*plr;
	char				 buf[512];

	fp = fopen(file, "r");
	if (fp == NULL) {
		warn("%s", file);
		return;
	}

	obj = object_create("player/digger");
	plr = obj->v.player = calloc(1, sizeof(struct player));
	plr->object = obj;
	env = object_find("room/1");
	object_reparent(obj, env);
	tellp(plr, "Digging");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		buf[strcspn(buf, "\r\n")] = '\0';
		player_input(plr, buf);
	}

	fclose(fp);
	object_free(obj);
}

int
main(int argc, char *argv[])
{
	struct event			*ev;
	struct evsrc			*fdsrc, *timersrc;
	int				 fd;

	fd = tcpbind("*", 4000);

	ev = event_create();
	if (ev == NULL)
		err(1, "event_create");

	fdsrc = evsrc_create_fd(fd, server_accept, NULL);
	if (fdsrc == NULL)
		err(1, "evsrc_create_fd");

	timersrc = evsrc_create_timer(10000, timercb, NULL);
	if (timersrc == NULL)
		err(1, "evsrc_create_timer");

	if (event_add_evsrc(ev, fdsrc) != 0)
		err(1, "event_add_evsrc");

	if (event_add_evsrc(ev, timersrc) != 0)
		err(1, "event_add_evsrc");

	load_world("rooms.txt");

	for (;;)
		if (event_dispatch(ev) == -1)
			err(1, "event_dispatch");

	evsrc_free(fdsrc);
	evsrc_free(timersrc);
	event_free(ev);
	return 0;
}
