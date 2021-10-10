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

#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>
#include <stdio.h>

struct player;
struct room;

typedef enum objtype {
	OBJ_TYPE_PLAYER=0,
	OBJ_TYPE_ROOM,
	OBJ_TYPE_ITEM,
	MAX_OBJ_TYPE
} ObjType;

#define IS_PLAYER(_x)		((_x)->type == OBJ_TYPE_PLAYER)	
#define IS_ROOM(_x)		((_x)->type == OBJ_TYPE_ROOM)
#define IS_ITEM(_x)		((_x)->type == OBJ_TYPE_ITEM)
#define OPARENT(_x)		(_x)->parent
#define OBJ(_x)			(_x)->object
#define PPARENT(_x)		OBJ(_x)->parent
#define PLAYER(_x)		(_x)->v.player
#define ROOM(_x)		(_x)->v.room
#define ENV(_x)			OBJ(_x)->parent
#define THIS(_x)		this_player(_x)

struct object {
	ObjType				 type;
	struct object			*parent;
	struct object			*first_child;
	struct object			*next;
	struct object			*next_all;
	struct object			*next_hash;
	char				*key;
	char				*title;
	union {
		struct player		*player;
		struct room		*room;
	} v;
};

void					 set_title(
					    struct object *,
					    const char *);
const char				*title(
					    struct object *);
size_t					 max_object_id(ObjType);

void					 object_save_all(
					    FILE *,
					    const char *);
struct object				*object_find(
					    const char *);
struct object				*object_create(
					    const char *);
void					 object_free(
					    struct object *);
void					 object_reparent(
					    struct object *,
					    struct object *);
struct object				*object_next_child(
					    struct object *,
					    struct object *);
struct object				*object_next(
					    struct object *);

#endif
