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

#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>

#include "evsrc.h"
#include "fmtbuf.h"

#define READ_BLOCK 8096
#define WRITE_CHUNK 8096

struct room;
struct object;

struct player {
	char		 buf[READ_BLOCK];
	int		 sz;
	struct evsrc	*evsrc;
	struct evsrc	*evwrite;

	/*
	 * 'outbuf' is a buffer for sending data to kernel's socket
	 * buffer. Impartial writes to the socket buffer might be
	 * possible so we need to retain some buffer contents here.
	 */
	char		*outbuf;
	size_t		 outbuf_alloc;
	size_t		 outbuf_sz;
	size_t		 outbuf_pos;

	/*
	 * 'herebuf' is a buffer for storing intermediate data similar
	 * to shell's here-documents. Here this means commands that
	 * ends with '<'.
	 */
	char		*herebuf;
	size_t		 herebuf_alloc;
	size_t		 herebuf_sz;
	char		*herebuf_cmdstr;

	/*
	 * 'object' backpointer is used for converting this struct
	 * back to its super-class via the OBJ() macro.
	 */
	struct object	*object;

	struct fmtbuf	fmtbuf;
};

struct room		*player_env(struct player *);

struct player		*this_player();

void			 	 	 player_input(
					    struct player *,
					    char *);
struct player				*player_create(
					    void);
void					 player_free(
					    struct player *);

#endif
