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

#include "../room.h"
#include "../player.h"
#include "../message.h"
#include "../tell.h"
#include "../args.h"
#include "../object.h"

#include <string.h>
#include <stdlib.h>

/*
 * dig [to:3] KEY [reverse]
 * Examples:
 *   dig n
 *   dig north
 *   dig to:3 north
 *   dig to:area/3 north
 *   dig to:area/3 north south
 *   dig inn out
 *   dig to:area/3 inn out
 */

#ifndef ARRLEN
#define ARRLEN(_x) sizeof((_x)) / sizeof((_x)[0])
#endif

#include <stdio.h>
static const char *
reverse_dir(const char *dir)
{
	static const struct {
		const char *dir;
		const char *reverse;
	} reverse[] = {
		{ "north", "south" },
		{ "up", "down" },
		{ "down", "up" },
		{ "west", "east" },
		{ "south", "north" },
		{ "east", "west" },
		{ "northwest", "southeast" },
		{ "northeast", "southwest" },
		{ "southwest", "northeast" },
		{ "southeast", "northwest" }
	};
	size_t				 i;

	for (i = 0; i < ARRLEN(reverse); i++) {
		if (strcmp(dir, reverse[i].dir) == 0)
			return reverse[i].reverse;
	}

	return NULL;
}

void
dig_main(struct player *plr, char *str)
{
	static const struct {
		const char *alias;
		char *str;
	} aliases[] = {
		{ "n", "north" },
		{ "u", "up" },
		{ "d", "down" },
		{ "w", "west" },
		{ "s", "south" },
		{ "e", "east" },
		{ "nw", "northwest" },
		{ "ne", "northeast" },
		{ "sw", "southwest" },
		{ "se", "southeast" }
	};

	size_t				 argc, i, j, k;
	char				*v[3];
	const char			*rev;
	struct object			*dest = NULL;

	if (!IS_ROOM(ENV(plr))) {
		tellpf(plr, "You cannot dig here.");
		return;
	}

	j = 0;
	argc = parse_args(str, v, 3);
	if (argc < 1) {
		tellp(plr, "Dig where?");
		return;
	}
	if (*v[j] == '?') {
		tellp(plr, "Direction or room ID.");
		return;
	}
	if (strncmp(v[j], "to:", 3) == 0) {
		if (strchr(v[j], '?') != NULL) {
			tellp(plr, "Room ID.");
			return;
		}
		dest = object_find(&v[j][3]);
		j++;
		argc--;
	}

	if (argc < 1) {
		tellp(plr, "Dig where?");
		return;
	}
	if (strchr(v[j], '?') != NULL) {
		tellp(plr, "Direction.");
		return;
	}

	rev = NULL;
	if (argc == 2 && strchr(v[j+1], '?') != NULL) {
		tellp(plr, "Reverse direction or "
		    "'-' to prevent automatic reversing.");
		return;
	} else if (argc == 2)
		rev = v[j+1];

	for (k = 0; k < argc; k++) {
		for (i = 0; i < ARRLEN(aliases); i++) {
			if (strcmp(v[j+k], aliases[i].alias) == 0) {
				v[j+k] = aliases[i].str;
				break;
			}
		}
	}

	if (rev == NULL && (rev = reverse_dir(v[j])) == NULL) {
		tellp(plr, "No reverse direction.");
	}
	if (rev != NULL && *rev == '-')
		rev = NULL;

	{
		char buf[80];
		struct object *another_room;

		if (dest == NULL) {
			snprintf(buf, sizeof(buf), "room/%zu",
			    max_object_id(OBJ_TYPE_ROOM) + 1);
			dest = object_find(buf);
		}

		if (room_add_exit(player_env(plr), v[j], dest->key) == -1)
			tellpf(plr, "Cannot dig the way to %s.", v[j]);

		if (rev != NULL) {
			another_room = dest;
			room_add_exit(ROOM(another_room), rev,
			    OBJ(player_env(plr))->key);
		}
		tellr(dest, NULL, "Something is changed.");
	}

	tellr(ENV(plr), plr, "Someone digged.");
}
