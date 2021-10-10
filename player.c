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

#include "player.h"
#include "object.h"
#include "match.h"
#include "command.h"
#include "util.h"
#include "tell.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <err.h>

struct player *
this_player()
{
	return NULL;
}

struct player *
player_create()
{
	struct player *plr;
	struct object *obj, *env;

	obj = object_create("player/1");
	plr = obj->v.player = calloc(1, sizeof(struct player));
	plr->object = obj;
	env = object_find("room/1");
	printf("Found env: %ju\n", (uintmax_t) env);
	object_reparent(obj, env);

	return plr;
}

struct room *
player_env(struct player *plr)
{
	return ROOM(OPARENT(OBJ(plr)));
}

void
player_free(struct player *plr)
{
	free(plr);
}

static const struct {
	const char *verb;
	void (*cmd)(struct player *, char *);
	ObjType env_req;
} cmds[] = {
	{ "describe", describe_main, 0 },
	{ "look", look_main, 0 },
	{ "go", go_main, OBJ_TYPE_ROOM },
	{ "dig", dig_main, OBJ_TYPE_ROOM },
	{ "collapse", collapse_main, OBJ_TYPE_ROOM },
	{ "say", say_main, 0 },
	{ "clear", clear_main, 0 },
	{ "goto", goto_main, 0 },
	{ "objects", objects_main, 0 }
};

const char **
mkverbs(struct player *plr, size_t *nverbs)
{
	static const char *verbs[ARRLEN(cmds)];
	size_t i;

	for (i = 0; i < ARRLEN(cmds); i++) {
		if (cmds[i].env_req != 0 && ENV(plr)->type != cmds[i].env_req)
			verbs[i] = NULL;
		else
			verbs[i] = cmds[i].verb;
	}

	*nverbs = ARRLEN(cmds);
	return verbs;
}

void
player_input(struct player *plr, char *str)
{
	static const struct {
		const char *alias;
		char *str;
	} aliases[] = {
		{ "n", "go north" },
		{ "s", "go south" },
		{ "w", "go west" },
		{ "e", "go east" },
		{ "nw", "go northwest" },
		{ "ne", "go northeast" },
		{ "sw", "go southwest" },
		{ "se", "go southeast" },
		{ "u", "go up" },
		{ "d", "go down" }
	};
	char *p = NULL, *q;
	size_t i, sz;
	ssize_t m;
	size_t len;
	const char **verbs;
	size_t nverbs;

	while (isspace(*str))
		str++;

	verbs = mkverbs(plr, &nverbs);

	/*
	 * Herebuf processing.
	 */
	len = strlen(str);
	if (len >= 2 && str[len-1] == '<' && str[len-2] == ' ') {
		str[--len] = '\0';
		plr->herebuf_cmdstr = strdup(str);
		return;
	}
	if (plr->herebuf_cmdstr != NULL) {
		if (len == 1 && str[0] == '.') {
			sz = plr->herebuf_sz + strlen(plr->herebuf_cmdstr) + 1;
			str = malloc(sz * sizeof(char));
			snprintf(str, sz, "%s%s", plr->herebuf_cmdstr,
			    plr->herebuf);

			free(plr->herebuf_cmdstr);
			plr->herebuf_cmdstr = NULL;
			free(plr->herebuf);
			plr->herebuf = NULL;
			plr->herebuf_sz = 0;
			plr->herebuf_alloc = 0;

			player_input(plr, str);
			free(str);
			return;
		} else {
			if (plr->herebuf_sz + len + 2 >= plr->herebuf_alloc) {
				if (plr->herebuf_alloc == 0)
					plr->herebuf_alloc = 512;
				else
					plr->herebuf_alloc *= 2;
				plr->herebuf = realloc(plr->herebuf,
				    plr->herebuf_alloc);
			}
			if (plr->herebuf_sz > 0)
				plr->herebuf_sz += snprintf(
				    &plr->herebuf[plr->herebuf_sz], len + 2,
				    " %s", str);
			else
				plr->herebuf_sz += snprintf(
				    &plr->herebuf[plr->herebuf_sz], len + 2,
				    "%s", str);
			return;
		}
	}

	/*
	 * Normal command processing.
	 */
	p = str;
	while (p != NULL && isspace(*p))
		p++;
	q = strchr(p, ' ');
	if (q != NULL)
		*q = '\0';

	for (i = 0; i < ARRLEN(aliases); i++) {
		if (strcmp(aliases[i].alias, p) == 0) {
			if (q != NULL) {
				sz = strlen(aliases[i].str) + 1 +
				    strlen(q) + 1;
				p = malloc(sz * sizeof(char));
				if (p != NULL)
					snprintf(p, sz, "%s %s",
					    aliases[i].str, q);
			} else
				p = strdup(aliases[i].str);

			if (p != NULL) {
				player_input(plr, p);
				free(p);
			} else
				warn("tmp buffer for alias");
			return;
		}
	}

	m = match_input(plr, NULL, p, verbs, nverbs);
	if (m >= 0) {
		if (cmds[m].cmd != NULL)
			cmds[m].cmd(plr, (q != NULL) ? ++q : NULL);
		else
			tellp(plr, "Nothing happens.\n");
	}

	if (q != NULL)
		*q = ' ';

	end_fmtbuf(&plr->fmtbuf);

	return;
}
