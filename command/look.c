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

void
look_main(struct player *plr, const char *str)
{
	struct object *obj;
	size_t i;

	if (!IS_ROOM(ENV(plr))) {
		tellpf(plr, "This location: %s.", ENV(plr)->key);
		return;
	}

#if 0
	tellpf(plr, "This location: %s\n", PPARENT(plr)->key);
#endif
	if (title(PPARENT(plr)) != NULL)
		tellpf(plr, "%s", title(PPARENT(plr)));

	highlight(&plr->fmtbuf, (const char **) player_env(plr)->exit_keys,
	    player_env(plr)->nexits);
	for (i = 0; i < player_env(plr)->nexits; i++) {
#if 0
		tellpf(plr, "Travel: %s -> %s\n",
		    player_env(plr)->exit_keys[i],
		    travel_desc(player_env(plr),
		    player_env(plr)->exit_keys[i]));
#endif
		tellpf(plr, "%s",
		    exit_desc(player_env(plr), player_env(plr)->exit_keys[i]));
#if 0
		tellpf(plr, "Exit: %s -> %s\n", player_env(plr)->exit_keys[i],
		    player_env(plr)->exit_targets[i]);
#endif
	}
	highlight(&plr->fmtbuf, NULL, 0);

	obj = NULL;
	while ((obj = object_next_child(OBJ(player_env(plr)), obj)) != NULL) {
		if (obj == OBJ(plr))
			continue;
		tellpf(plr, "There is %s here.", obj->key);
	}
}
