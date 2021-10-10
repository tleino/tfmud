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

#include "../command.h"
#include "../room.h"
#include "../player.h"
#include "../match.h"
#include "../message.h"
#include "../object.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef ARRLEN
#define ARRLEN(_x) sizeof((_x)) / sizeof((_x)[0])
#endif

enum target_types {
	TARGET_NONE=0, TARGET_DIR, TARGET_DETAIL
};

void
describe_main(struct player *plr, char *str)
{
	static const char *desc_types[] = {
		"travel", "exit", "arrive", "leave", "title", "detail", "save"
	};
	static int target[] = {
		TARGET_DIR, TARGET_DIR, TARGET_DIR, TARGET_DIR,
		TARGET_NONE, TARGET_DETAIL, TARGET_NONE
	};
	int m, i;
	size_t argc;
	char *v1[2], *v2[2];

	argc = parse_args(str, v1, 2);
	if (argc < 1) {
		tellp(plr, "Describe what?\n");
		return;
	}
	m = match_input(plr, NULL, v1[0], desc_types, ARRLEN(desc_types));
	if (m == -1)
		return;
	if (target[m] != TARGET_NONE) {
		if (argc < 2) {
			if (target[m] == TARGET_DIR)
				tellp(plr, "Which direction?\n");
			else
				tellp(plr, "Which detail?\n");
			return;
		}
		argc = parse_args(v1[1], v2, 2);
		i = match_input(plr, NULL, v2[0],
		    (const char **) player_env(plr)->exit_keys,
		    player_env(plr)->nexits);
		if (i == -1)
			return;
		if (argc < 2) {
			tellp(plr, "And the description please?\n");
			return;
		}

		if (m == DESC_ENTER)
			set_travel_desc(player_env(plr), v2[0], v2[1]);
		else if (m == DESC_EXIT)
			set_exit_desc(player_env(plr), v2[0], v2[1]);
		tellp(plr, "Description updated.");
	} else if (strcmp(desc_types[m], "save") == 0) {
		FILE *fp = fopen("rooms.txt", "w");
		object_save_all(fp, "room/*");
		fclose(fp);
		tellp(plr, "Saved.");
	} else if (strcmp(desc_types[m], "title") == 0) {
		set_title(PPARENT(plr), v1[1]);
		tellp(plr, "Title updated.");
	}
}
