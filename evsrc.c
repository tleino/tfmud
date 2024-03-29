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

#include <stdlib.h>

static struct evsrc	*evsrc_create(EvSrcType, int,
			    int (*)(struct evsrc *, void *), void *);

struct evsrc *
evsrc_create_fd(int fd, int (*readcb)(struct evsrc *, void *), void *data)
{
	return evsrc_create(EVSRC_FD, fd, readcb, data);
}

struct evsrc *
evsrc_create_write_fd(int fd, int (*readcb)(struct evsrc *, void *),
    void *data)
{
	return evsrc_create(EVSRC_WRITE_FD, fd, readcb, data);
}

struct evsrc *
evsrc_create_timer(int timeout, int (*readcb)(struct evsrc *, void *),
    void *data)
{
	return evsrc_create(EVSRC_TIMER, timeout, readcb, data);
}

static struct evsrc *
evsrc_create(EvSrcType type, int value,
    int (*readcb)(struct evsrc *, void *), void *data)
{
	struct evsrc *evsrc;

	evsrc = calloc(1, sizeof(struct evsrc));
	if (evsrc != NULL) {
		evsrc->type = type;
		evsrc->value = value;
		evsrc->readcb = readcb;
		evsrc->data = data;
	}

	return evsrc;
}

void
evsrc_free(struct evsrc *evsrc)
{
	free(evsrc);
}
