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

#include "fmtbuf.h"

#include <ctype.h>
#include <string.h>

#define sentence_end(_x) \
	((_x) == '?' || (_x) == '!' || (_x) == '.')
#define punct(_x) \
	((_x) == ',' || (_x) == ';' || (_x) == ':')
#define word_end(_x) \
	(sentence_end((_x)) || isspace((_x)) || punct((_x)))

void
end_fmtbuf(struct fmtbuf *fb)
{
	fb->outbuf[fb->j++] = '\n';
	fb->outbuf[fb->j++] = '\n';
	fb->outbuf[fb->j] = '\0';
	fb->state = BEGIN_WORD;
	fb->upper = 0;
	fb->len = 0;
	fb->rewind_j = 0;
	fb->wordlen = 0;
}

void
dump(struct fmtbuf *fb, const char *out, size_t outlen, int nolen)
{
	size_t i;

	for (i = 0; i < outlen; i++) {
		if (out[i] == '\n')
			fb->len = 0;
		else if (!nolen)
			fb->len++;
		fb->outbuf[fb->j++] = out[i];
	}
	fb->outbuf[fb->j] = '\0';
}

#include <stdio.h>
void
highlight(struct fmtbuf *fb, const char **words, size_t nwords)
{
	fb->words = words;
	fb->nwords = nwords;

	{
		size_t i;
		for (i = 0; i < fb->nwords; i++) {
			printf("Highlight %s\n", fb->words[i]);
		}
	}
}

void
add_fmtbuf_raw(struct fmtbuf *fb, const char *src)
{
	while (*src != '\0') {
		fb->outbuf[fb->j++] = *src++;
	}
	fb->outbuf[fb->j] = '\0';
}

void
add_fmtbuf(struct fmtbuf *fb, const char *src)
{
	const char			*p;
	char				 tmp[3], *out;
	size_t				 outlen, i;
	int				 split, end;
	int				 match;

	if (fb->len == 0) {
		dump(fb, "     ", 5, 1);
	}

	for (p = src; *p != '\0'; ) {
		outlen = 0;
		out = tmp;
		switch (fb->state) {
		case BEGIN_WORD:
			if (*p == '\"' || *p == '\'') {
				dump(fb, p, 1, 0);
				p++;
				break;
			}
			if (isspace(*p)) {
				p++;
				break;
			}
			fb->state = IN_WORD;
			fb->wordlen = 0;
			/* FALLTHROUGH */
		case IN_WORD:
			if (!word_end(*p) && fb->wordlen + 1 <
			    sizeof(fb->word)) {
				fb->word[fb->wordlen++] = *p++;
				break;
			}
			split = 0;
			if (fb->len + fb->wordlen >= 65) {
				tmp[0] = '\n';
				outlen = 1;
				split = 1;
				dump(fb, tmp, outlen, 0);
				dump(fb, "     ", 5, 1);
			}
			fb->word[fb->wordlen] = '\0';
			if (fb->word[0] != '\0' && fb->upper) {
				fb->word[0] = toupper(fb->word[0]);
				fb->upper = 0;
			}

			match = 0;
			if (fb->words != NULL && fb->nwords > 0) {
				for (i = 0; i < fb->nwords; i++) {
					if (strcmp(fb->words[i],
					    fb->word) == 0) {
						match = 1;
						break;
					}
				}
			}
			if (match)
				dump(fb, "\033[1;31m", 4 + 3, 1);

			dump(fb, fb->word, fb->wordlen, 0);
			if (match)
				dump(fb, "\033[0m", 4, 1);

			tmp[0] = *p;
			outlen = 1;
			dump(fb, tmp, outlen, 0);
			if (fb->j > 0 && fb->outbuf[fb->j-1] == ' ')
				fb->rewind_j = fb->j-1;
			else
				fb->rewind_j = fb->j;

			outlen = 0;
			end = sentence_end(*p);
			if (end)
				fb->upper = 1;
			if (*(p+1) != '\0' &&
				(*(p+1) == '\"' || *(p+1) == '\'')) {
				if (end) {
					tmp[outlen++] = *++p;
					tmp[outlen++] = ' ';
					tmp[outlen++] = ' ';
				}
			} else if (end) {
				tmp[outlen++] = ' ';
				tmp[outlen++] = ' ';
			} else if (!isspace(*p)) {
				tmp[outlen++] = ' ';
			}
			dump(fb, tmp, outlen, 0);
			fb->state = AFTER_WORD;
			p++;
			break;
		case AFTER_WORD:
			if (!(sentence_end(*p) || punct(*p))) {
				fb->state = BEGIN_WORD;
				break;
			} else
				p++;
			break;
		}
	}
}
