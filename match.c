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

#include "match.h"
#include "player.h"
#include "message.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * match_input:
 *   Match player input with available options that can be used for
 *   parsing commands, subcommands, subsubcommands, commands taking
 *   environment objects as targets, etc.
 *
 *   This requires actual player object because only players need this
 *   convenience feature, and only players may have set up aliases for
 *   the commands.
 * 
 *   In case of match failure, the player is told what went wrong and
 *   -1 is returned.
 *
 *   In case of success, the matching option is returned.
 *
 *   Example:
 *     match_input(plr, "l", "elffi", { "human", "elf", NULL }, 2);
 *     == "Did you mean 'l elffi'?"
 *
 *     match_input(plr, NULL, "loak", { "look", "who", NULL }, 2);
 *     == 'Did you mean 'look?'"
 */
struct match_result {
	const char	*candidate;
	int		 matches;
	int		 dist;
};

static int				 levenshtein_distance(const char *,
					    size_t, const char *, size_t);

static int
compare_matches(const void *_a, const void *_b)
{
	const struct match_result	*a = _a;
	const struct match_result	*b = _b;

	if (a->matches < b->matches)
		return 1;
	else if (a->matches == b->matches) {
		if (a->candidate != NULL && b->candidate != NULL)
			return strcmp(a->candidate, b->candidate);
		else
			return 0;
	} else
		return -1;
}

static int
compare_levenshtein(const void *_a, const void *_b)
{
	const struct match_result	*a = _a;
	const struct match_result	*b = _b;

	if (a->dist > b->dist)
		return 1;
	else if (a->dist == b->dist) {
		if (a->candidate != NULL && b->candidate != NULL)
			return strcmp(a->candidate, b->candidate);
		else
			return 0;
	} else
		return -1;
}

int
match_input(struct player *plr, const char *lstr, char *str,
    const char **opt, size_t nopt)
{
	struct match_result		 res[nopt];
	size_t				 i;
	const char			*p, *q;
	int				 want_list;
	size_t				 len;

	len = strlen(str);
	if (len == 0 || nopt == 0)
		return -1;

	want_list = 0;
	for (i = 0; i < nopt; i++) {
		p = str;
		q = opt[i];
		res[i].candidate = q;
		res[i].matches = 0;
		if (q == NULL)
			continue;
		while (*p != '\0' && *q != '\0') {
			if (*p == '?') {
				want_list = 1;
				break;
			} else if (tolower(*q++) == tolower(*p++)) {
				res[i].matches++;
			} else {
				res[i].matches = 0;
				break;
			}
		}

		/*
		 * If we typed longer word than expected...
		 */
		if (*q == '\0' && *p != '\0')
			res[i].matches = 0;
	}

	qsort(res, nopt, sizeof(*res), compare_matches);

	/*
	 * Check for non-ambiguous.
	 */
	if (!want_list && res[0].matches > 0 &&
	    (nopt <= 1 || res[1].matches != res[0].matches ||
	    (res[0].candidate != NULL &&
	    (int) strlen(res[0].candidate) == res[0].matches)))
		for (i = 0; i < nopt; i++)
			if (res[0].candidate == opt[i])
				return i;

	/*
	 * List ambiguous.
	 */
	if (want_list || res[0].matches > 0) {
		for (i = 0; i < nopt; i++) {
			if (res[0].matches == res[i].matches &&
			    res[i].candidate != NULL)
				tellpf(plr, "%c%s%s",
				    toupper(res[i].candidate[0]),
				    &res[i].candidate[1],
				    want_list ? ".\n" : "?\n");
		}
	} else {
		if (len > 10) {
			str[0] = toupper(str[0]);
			tellpf(plr, "%s?\n", str);
		} else {
			for (i = 0; i < nopt; i++) {
				if (res[i].candidate == NULL)
					res[i].dist = 10000;
				else
					res[i].dist = levenshtein_distance(
					    res[i].candidate,
					    strlen(res[i].candidate), str,
					    len);
			}

			qsort(res, nopt, sizeof(*res), compare_levenshtein);
			tellpf(plr, "Try %s?\n", res[0].candidate);
		}
	}

	return -1;
}

static int
levenshtein_distance(const char *s1, size_t len1, const char *s2, size_t len2)
{
	size_t			 matrix[len1 + 1][len2 + 1];
	size_t			 i, j;
	size_t			 del, ins, sub, min;
	char			 c1, c2;

	for (i = 0; i <= len1; i++)
		matrix[i][0] = i;
	for (i = 0; i <= len2; i++)
		matrix[0][i] = i;
	for (i = 1; i <= len1; i++) {
		c1 = s1[i-1];
		for (j = 1; j <= len2; j++) {
			c2 = s2[j-1];
			if (c1 == c2)
				matrix[i][j] = matrix[i-1][j-1];
			else {
				del = matrix[i-1][j] + 1;
				ins = matrix[i][j-1] + 1;
				sub = matrix[i-1][j-1] + 1;
				min = del;
				if (ins < min)
					min = ins;
				if (sub < min)
					min = sub;
				matrix[i][j] = min;
			}
		}
	}
	return matrix[len1][len2];
}
