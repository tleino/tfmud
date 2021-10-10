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

#ifndef TELL_H
#define TELL_H

#include <stdarg.h>

struct player;
struct room;
struct object;

/*
 * tellp:	tell player
 * tellpf:	tell player	(formated)
 * tellr:	tell room	(single exclude)
 * tellrm:	tell room	(multiple exclude)
 * tellrf:	tell room	(formated, single exclude)
 * tellrfm:	tell room	(formated, multiple exclude)
 */
void					 tellp(
					    struct player *,
					    const char *);
void					 tellpf(
					    struct player *,
					    const char *,
					    ...);
void					 tellr(
					    struct object *,
					    struct player *,
					    const char *);
void					 tellrm(
					    struct object *,
					    struct player **,
					    const char *);
void					 tellrf(
					    struct object *,
					    struct player *,
					    const char *,
					    ...);
void					 tellrfm(
					    struct object *,
					    struct player **,
					    const char *,
					    ...);

#endif
