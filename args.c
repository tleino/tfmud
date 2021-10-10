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

#include <ctype.h>
#include <string.h>

/*
 * Parse space separated string to array elements until reaching a limit, 
 * which can be the maximum array size, or smaller arbitrary limit where
 * rest of the string is squeezed to one array element.
 *
 * Returns the number of array elements used.
 * Modifies the supplied string.
 */
size_t
parse_args(char *p, char **argv, size_t limit)
{
	char				*q;
	size_t				 argc;

	argc = 0;
	while (p != NULL && (argc+1) < limit && *p != '\0') {
		while (isspace(*p))
			p++;
		q = strchr(p, ' ');
		if (q != NULL)
			*q++ = '\0';
		argv[argc++] = p;
		p = q;
		
	}
	if (p != NULL)
		argv[argc++] = p;

	return argc;
}

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
int
main(int argc, char **argv)
{
	size_t				 c, i;
	char				*v[3], *p;

	p = strdup("arg arg2 and a longer string");
	c = parse_args(p, v, 3);
	for (i = 0; i < c; i++)
		printf("v[%zu] = %s\n", i, v[i]);
	free(p);

	p = strdup("?");
	c = parse_args(p, v, 3);
	printf("c: %zu\n", c);
	for (i = 0; i < c; i++)
		printf("v[%zu] = %s\n", i, v[i]);
}
#endif
