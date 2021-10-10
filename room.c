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

#include "room.h"
#include "player.h"
#include "object.h"
#include "message.h"
#include "tell.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct desc
{
	char			*name;
	char			*text;
	int			 tags;
	size_t			 text_alloc;
};

#define DESC_CHUNK 4
#define LOC_CHUNK 32

#if 0
#define LOC_HASH_SIZE (65536 * 2)
#else
#define LOC_HASH_SIZE 8192
#endif

int nitems;
int nconflicts;

#include <math.h>

static size_t room_find_exit(struct room *room, const char *key);

/*
 * For internal usage.
 */
static char *
simple_wrap(const char *p)
{
	static char			 out[8192];
	size_t				 i, j;
	size_t				 len;

	len = i = j = 0;
	while (*p != '\0') {
		len++;
		out[i] = *p;
		if (isspace(*p))
			j = i;
		if (len >= 65) {
			p -= (i - j);
			i = j;
			out[i++] = '\n';
			out[i++] = '\t';
			len = 0;
			p++;
		}
		out[i] = *p;
		p++;
		i++;
	}
	out[i] = '\0';
	return out;
}

void
room_save(struct room *room, FILE *fp)
{
	size_t			 i;

	fprintf(fp, "goto %s\n", OBJ(room)->key);
	for (i = 0; i < room->nexits; i++) {
		fprintf(fp, "dig to:%s %s -\n", room->exit_targets[i],
		    room->exit_keys[i]);
	}
	fprintf(fp, "describe title <\n\t%s\n\t.\n",
	    simple_wrap(title(OBJ(room))));
	for (i = 0; i < room->nexits; i++) {
		fprintf(fp, "describe travel %s <\n\t%s\n\t.\n",
		    room->exit_keys[i], simple_wrap(travel_desc(room,
		    room->exit_keys[i])));
		fprintf(fp, "describe exit %s <\n\t%s\n\t.\n",
		    room->exit_keys[i], simple_wrap(exit_desc(room,
		    room->exit_keys[i])));
	}
	fprintf(fp, "\n");
}

const char *
room_exit_target(struct room *room, const char *key)
{
	size_t			 i;

	i = room_find_exit(room, key);
	if (i == MAX_EXITS)
		return 0;

	return room->exit_targets[i];
}

size_t
room_find_exit(struct room *room, const char *key)
{
	size_t			 i;

	for (i = 0; i < room->nexits; i++)
		if (room->exit_keys[i] != NULL &&
		    strcmp(room->exit_keys[i], key) == 0)
			break;

	return i;
}

int
room_add_exit(struct room *room, const char *key, const char *target)
{
	size_t			 i;

	if (room->nexits == MAX_EXITS)
		return -1;
	if (room_find_exit(room, key) != room->nexits)
		return -1;

	i = room->nexits++;
	if ((room->exit_keys[i] = strdup(key)) == NULL)
		return -1;
	if ((room->exit_targets[i] = strdup(target)) == NULL)
		return -1;

	tellrf(OBJ(room), NULL, "A way to %s appears leads to %s.", key,
	    target);

	return 0;
}

int
room_remove_exit(struct room *room, const char *key)
{
	size_t			 i;

	if ((i = room_find_exit(room, key)) == room->nexits)
		return -1;

	room->nexits--;
	if (room->nexits > i) {
		if (room->exit_keys[i] != NULL)
			free(room->exit_keys[i]);
		if (room->exit_targets[i] != NULL)
			free(room->exit_targets[i]);
		memmove(&room->exit_keys[i], &room->exit_keys[i+1],
		    sizeof(char *) * (room->nexits - i));
		memmove(&room->exit_targets[i], &room->exit_targets[i+1],
		    sizeof(char *) * (room->nexits - i));
		memmove(&room->exit_travel_desc[i],
		    &room->exit_travel_desc[i+1],
		    sizeof(char *) * (room->nexits - i));
		memmove(&room->exit_desc[i], &room->exit_desc[i+1],
		    sizeof(char *) * (room->nexits - i));
	}
	tellrf(OBJ(room), NULL, "A way to %s disappears.", key);
	return 0;
}

struct room *
room_create(struct object *obj, const char *id)
{
	obj->v.room = calloc(1, sizeof(struct room));
	obj->v.room->object = obj;

	return obj->v.room;
}

int
room_bytes(struct room *room)
{
	int i, j, bytes;

	bytes = sizeof(struct room);
	for (i = 0; i < MAX_DESC_TYPES; i++) {
		bytes += room->alloc_desc[i] * sizeof(struct desc);
		for (j = 0; j < room->n_desc[i]; j++) {
			bytes += room->desc[i][j].text_alloc;
		}
	}

	return bytes;
}

void
room_free(struct room *room)
{
	free(room);
}

const char *
room_desc(struct room *room, DescType type)
{
	if (room->desc[type] == NULL)
		return "nothing";

	return room->desc[type][0].text;
}

void
add_desc(struct room *room, DescType type, const char *str)
{
	struct desc *d;
	char *p;

	if (room->n_desc[type] == room->alloc_desc[type]) {
		if (room->alloc_desc[type] == 0)
			room->alloc_desc[type] = DESC_CHUNK;
		else
			room->alloc_desc[type] *= 2;
		d = realloc(room->desc[type],
		    sizeof(struct desc) * room->alloc_desc[type]);
		if (d == NULL) {
			err(1, "realloc, tried +%lu bytes",
			    sizeof(struct desc) * room->alloc_desc[type]);
			return;
		}
		room->desc[type] = d;
	}

	p = strdup(str);
	if (p == NULL) {
		err(1, "strdup");
		return;
	}
	room->desc[type][room->n_desc[type]].text = p;
	room->desc[type][room->n_desc[type]].text_alloc = strlen(str) + 1;
	room->n_desc[type] = 0;

	tellrf(OBJ(room), NULL, "Something here seems different.");
}

const char *
travel_desc(struct room *room, const char *dkey)
{
	size_t				 i;

	if ((i = room_find_exit(room, dkey)) == room->nexits)
		return NULL;

	return room->exit_travel_desc[i];
}

const char *
exit_desc(struct room *room, const char *dkey)
{
	size_t				 i;

	if ((i = room_find_exit(room, dkey)) == room->nexits)
		return NULL;

	return room->exit_desc[i];
}

void
set_travel_desc(struct room *room, const char *dkey, const char *str)
{
	size_t				 i;

	tellrf(OBJ(room), NULL, "Something changes in %s.", dkey);

	if ((i = room_find_exit(room, dkey)) == room->nexits)
		return;

	if ((room->exit_travel_desc[i] = strdup(str)) == NULL)
		return;
}

void
set_exit_desc(struct room *room, const char *dkey, const char *str)
{
	size_t				 i;

	tellrf(OBJ(room), NULL, "Something changes in %s.", dkey);

	if ((i = room_find_exit(room, dkey)) == room->nexits)
		return;

	if ((room->exit_desc[i] = strdup(str)) == NULL)
		return;
}
