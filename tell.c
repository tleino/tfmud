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

#include "tell.h"
#include "player.h"
#include "evsrc.h"
#include "event.h"
#include "object.h"
#include "room.h"
#include "fmtbuf.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

static int	client_write(struct evsrc *src, void *data);

static void				 tellpfv(
					    struct player *,
					    const char *,
					    va_list);
#include <string.h>

#if 0
static void
flush_fmtbuf(struct player *plr)
{
	ssize_t				 n;
	char				*p;

	if (plr->outbuf == NULL) {
		plr->outbuf = malloc(8192 * sizeof(char));
		plr->outbuf_alloc = 8192;
		plr->outbuf_sz = 0;
		return 0;
	}

	p = plr->fmtbuf;
	while (*p != '\0') {
		if (plr->outbuf_sz == plr->outbuf_alloc)
			break;
		plr->outbuf[plr->outbuf_sz++] = *p++;
	}
	n = write(plr->evsrc->value, plr->outbuf, plr->outbuf_sz);
	if (n == plr->outbuf_sz)
		plr->outbuf_sz = 0;
	else if (n > 0) {
		memmove(plr->outbuf, &plr->outbuf[n],
		    (plr->outbuf_sz - n) * sizeof(char));
		plr->outbuf_sz -= n;
		event_add_evsrc(plr->evsrc->ev, plr->evwrite);
	}
}

static void
grow_fmtbuf(struct player *plr)
{
	char				*p;
	size_t				 alloc;

	if (plr->fmtbuf_alloc == 0)
		alloc = 512;
	else
		alloc *= 2;

	if ((p = realloc(plr->fmtbuf, alloc * sizeof(char))) == NULL) {
		plr->fmtbuf_sz = 0;
	} else {
		plr->fmtbuf = p;
		plr->fmtbuf_alloc = alloc;
	}

	printf("Grow fmtbuf to %zu\n", plr->fmtbuf_alloc);
}

static void
add_fmtbuf(struct player *plr, const char *src)
{
	char				 word[128], *p, *end;
	size_t				 i, j;
	char				 outbuf[8192];

	i = 0;
	p = word;
	end = &word[127];
	j = 0;
	while (*src != '\0') {
		if (p == end ||
		    *src == ' ' || *src == '.' || *src == '?' || *src == '!') {
			*p = '\0';
			printf("Word: %s\n", word);
			outbuf[j] = '@';
			p = word;
			while (*p != '\0')
				outbuf[j++] = *p;
			outbuf[j++] = '@';
			outbuf[j++] = *src;
			p = word;
		}
		*p++ = *src++;
		word[i] = *src;	
	}
	outbuf[j] = '\0';
	printf("Outbuf: %s\n", outbuf);
#if 0
	while (*src != '\0') {
		if (plr->fmtbuf_sz == plr->fmtbuf_alloc)
			grow_fmtbuf(plr);
		plr->fmtbuf[plr->fmtbuf_sz++] = *src++;
	}
#endif
}

static size_t
wrap_buf(char *buf)
{
	size_t len;
	char *p, *sp;
	int last_was_white;

	printf("Wrapping this:\n\"%s\"\n", buf);

	p = buf;
	sp = NULL;
	len = 0;
	last_was_white = 0;
	while (p != NULL && *p != '\0') {
		if (*p == '\n') {
			sp = NULL;
			len = 0;
			last_was_white = 1;
		}
		if (*p == ' ' && last_was_white == 0) {
			sp = p;
			last_was_white = 1;
		} else if (*p != ' ' && *p != '\n') {
			last_was_white = 0;
		}
		len++;
		if (len >= 65 && sp != NULL) {
			printf("len is: %zu\n", len);
			*sp = '\n';
			p = sp;
		} else
			p++;
	}
	if (*p == '\0') {
		printf("At end, at pos %zu\n", p - buf);
		while (p != buf && *(p-1) == ' ') {
			printf("decr p\n");
			p--;
		}
		if (*p != '\0') {
			*p++ = '\n';
		}
	}
	return p - buf;
}
#endif

static int
client_write(struct evsrc *src, void *data)
{
	struct player *plr = (struct player *) data;

	write(plr->evsrc->value, plr->fmtbuf.outbuf,
	    strlen(plr->fmtbuf.outbuf));
	plr->fmtbuf.j = 0;
	plr->fmtbuf.len = 0;
	plr->fmtbuf.state = BEGIN_WORD;
	plr->fmtbuf.upper = 0;

	/*
	 * TODO: Add back the support for partial writes.
	 */
#if 0
	if (plr->outbuf == NULL)
		return 0;

	p = &plr->outbuf[plr->outbuf_pos];
	bsz = plr->outbuf_sz - plr->outbuf_pos;

	plr->outbuf[plr->outbuf_sz] = '\0';

	bsz = plr->outbuf_sz - plr->outbuf_pos;

	for (off = 0; off < bsz; off += nw) {
		nw = write(plr->evsrc->value, p + off, bsz - off);
		printf("Wrote %zd bytes\n", nw);
		if (nw == 0 || nw == -1) {
			warn("write");
			break;
		}
	}
	plr->outbuf_pos += off;
	if (plr->outbuf_pos == plr->outbuf_sz) {
		printf("Full message written\n");
		plr->outbuf_sz = 0;
		plr->outbuf_pos = 0;
	}
#endif

	return 0;
}

void
tellp(struct player *plr, const char *msg)
{
	if (msg == NULL || plr->evsrc == NULL)
		return;

	if (plr->evwrite == NULL) {
		plr->evwrite = evsrc_create_write_fd(plr->evsrc->value,
		    client_write, plr);
		if (plr->evwrite == NULL) {
			warn("evsrc_create_write_fd");
			return;
		}
	}
	event_add_evsrc(plr->evsrc->ev, plr->evwrite);

	add_fmtbuf(&plr->fmtbuf, msg);
}

static void
tellpfv(struct player *plr, const char *fmt, va_list ap)
{
	static char			 buf[8192];
	size_t				 n;

	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (n >= sizeof(buf))
		warn("too long message in tellpfv");
	tellp(plr, buf);
}

void
tellpf(struct player *plr, const char *fmt, ...)
{
	va_list				 ap;

	va_start(ap, fmt);
	tellpfv(plr, fmt, ap);
	va_end(ap);
}

void
tellrm(struct object *room, struct player **excl, const char *buf)
{
	struct object			*obj;

	obj = NULL;
	while ((obj = object_next_child(room, obj)) != NULL)
		if (IS_PLAYER(obj)) {
			if (excl != NULL)
				for (; *excl != NULL; excl++)
					if (*excl == PLAYER(obj))
						break;
			if (excl == NULL || *excl == NULL) {
				tellp(PLAYER(obj), buf);
				end_fmtbuf(&PLAYER(obj)->fmtbuf);
			}
		}
}

void
tellr(struct object *room, struct player *plr, const char *buf)
{
	struct player			*excl[] = { plr, NULL };

	tellrm(room, excl, buf);
}

static void
tellrfv(struct object *room, struct player **excl, const char *fmt, va_list ap)
{
	static char			 buf[8192];
	size_t				 n;

	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (n >= sizeof(buf))
		warn("too long message in tellrfv");
	tellrm(room, excl, buf);
}

void
tellrfm(struct object *room, struct player **excl, const char *fmt, ...)
{
	va_list				 ap;

	va_start(ap, fmt);
	tellrfv(room, excl, fmt, ap);
	va_end(ap);
}

void
tellrf(struct object *room, struct player *plr, const char *fmt, ...)
{
	va_list				 ap;
	struct player			*excl[] = { plr, NULL };

	va_start(ap, fmt);
	tellrfv(room, excl, fmt, ap);
	va_end(ap);
}
