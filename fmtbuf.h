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

#ifndef FMTBUF_H
#define FMTBUF_H

#include <stddef.h>

enum fmtbuf_state {
	BEGIN_WORD=0, IN_WORD, AFTER_WORD
};

struct fmtbuf {
	char				 word[64];
	char				 outbuf[8192];
	size_t				 wordlen;
	size_t				 j;
	size_t				 rewind_j;
	size_t				 len;
	int				 state;
	int				 upper;
	const char			**words;
	size_t				 nwords;
};

void
highlight(struct fmtbuf *fb, const char **words, size_t nwords);

void add_fmtbuf_raw(struct fmtbuf *fb, const char *src);

void
add_fmtbuf(struct fmtbuf *fb, const char *src);
void
end_fmtbuf(struct fmtbuf *fb);

#endif
