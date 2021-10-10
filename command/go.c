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
#include "../object.h"
#include "../args.h"
#include "../match.h"

void
go_main(struct player *plr, char *str)
{
	size_t				 argc;
	char				*v[1];
	int				 m;
	size_t				 i;
	const char			*key, *target;
	struct object			*prev;

	if (!IS_ROOM(ENV(plr))) {
		tellpf(plr, "You cannot go anywhere here.");
		return;
	}

	argc = parse_args(str, v, 1);
	if (argc < 1) {
		tellp(plr, "Go where?\n");
		return;
	}
	m = match_input(plr, NULL, v[0],
	    (const char **) player_env(plr)->exit_keys,
	    player_env(plr)->nexits);
	if (m == -1)
		return;
	key = player_env(plr)->exit_keys[m];

	target = room_exit_target(ROOM(PPARENT(plr)), key);
	if (target == NULL) {
		tellp(plr, "Cannot go that way here.\n");
		return;
	}
	tellpf(plr, "%s  ", travel_desc(ROOM(PPARENT(plr)), key));

	tellrf(ENV(plr), plr, "%s leaves to %s.", OBJ(plr)->key, key);

	prev = PPARENT(plr);
	object_reparent(OBJ(plr), object_find(target));

	tellrf(ENV(plr), plr, "%s arrives.", OBJ(plr)->key);

	highlight(&plr->fmtbuf, (const char **) player_env(plr)->exit_keys,
	    player_env(plr)->nexits);
	for (i = 0; i < player_env(plr)->nexits; i++) {
		if (prev == object_find(player_env(plr)->exit_targets[i]))
			continue;
		tellpf(plr, "%s  ", exit_desc(ROOM(PPARENT(plr)),
		    player_env(plr)->exit_keys[i]));
	}
	highlight(&plr->fmtbuf, NULL, 0);
}
