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

#include "object.h"
#include "room.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <fnmatch.h>

static void				 object_add_child(
					    struct object *,
					    struct object *);
static void				 object_remove_child(
					    struct object *,
					    struct object *);
static void				 object_add(
					    struct object *);
static void				 object_remove(
					    struct object *);
static ObjType				 parse_type(
					    const char *);
static char				*parse_key(
					    const char *,
					    ObjType *);
static size_t				 hash_key(
					    const char *);

#define OBJ_HASH_SZ	8192

static struct object			*_head;
static struct object			*_hash[OBJ_HASH_SZ];
static size_t				 _max_id[MAX_OBJ_TYPE];

#ifndef ARRLEN
#define ARRLEN(_x) sizeof((_x)) / sizeof((_x)[0])
#endif

void
set_title(struct object *obj, const char *str)
{
	if (obj->title != NULL)
		free(obj->title);
	obj->title = strdup(str);
}

const char *
title(struct object *obj)
{
	return obj->title;
}

static size_t
hash_key(const char *key)
{
	size_t				 k;

	for (k = 0; *key != '\0'; key++)
		k = *key + (k << 6) + (k << 16) - k;
	k %= OBJ_HASH_SZ;

	return k;
}

static ObjType
parse_type(const char *type)
{
	static const char		*types[MAX_OBJ_TYPE] = {
		"player", "room", "item"
	};
	ObjType				 i;

	for (i = 0; i < MAX_OBJ_TYPE; i++)
		if (strcmp(types[i], type) == 0)
			return i;

	return i;
}

static char *
parse_key(const char *key, ObjType *type)
{
	char				 cl[16 + 1];
	static char			 id[16 + 1];
	size_t				 i;

	assert(key != NULL);

	i = strcspn(key, "/");
	assert(key[i] != '\0');

	/*
	 * Truncate to sizeof(cl)-1
	 */
	strncpy(cl, key, i > sizeof(cl)-1 ? sizeof(cl)-1 : i);
	cl[i > sizeof(cl)-1 ? sizeof(cl)-1 : i] = '\0';

	*type = parse_type(cl);
	strlcpy(id, &key[i+1], sizeof(id));

	return id;
}

#include <stdio.h>
struct object *
object_find(const char *key)
{
	struct object			*np;
	size_t				 k;

	k = hash_key(key);
	np = _hash[k];
	while (np != NULL) {
		if (strcmp(np->key, key) != 0)
			np = np->next_hash;
		else
			break;
	}
	if (np == NULL) {
		/* FIXME: Always return object */
		if ((np = object_create(key)) == NULL)
			return NULL;
		np->next_hash = _hash[k];
		_hash[k] = np;
	}

	return np;
}

size_t
max_object_id(ObjType type)
{
	return _max_id[type];
}

struct object *
object_create(const char *key)
{
	struct object			*obj;
	const char			*id;

	printf("Create object: %s\n", key);

	if ((obj = calloc(1, sizeof(*obj))) == NULL)
		return NULL;

	if ((obj->key = strdup(key)) == NULL) {
		free(obj);
		return NULL;
	}

	id = parse_key(key, &obj->type);
	if (_max_id[obj->type] < (size_t) atoll(id))
		_max_id[obj->type] = (size_t) atoll(id);

	if (obj->type == OBJ_TYPE_ROOM)
		room_create(obj, id);

	object_add(obj);
	return obj;
}

void
object_save_all(FILE *fp, const char *pattern)
{
	struct object			*obj;

	obj = NULL;
	while ((obj = object_next(obj)) != NULL) {
		if (fnmatch(pattern, obj->key, 0) == FNM_NOMATCH)
			continue;
		printf("Saving %s\n", obj->key);
		if (obj->type == OBJ_TYPE_ROOM)
			room_save(ROOM(obj), fp);
	}
}

void
object_free(struct object *obj)
{
	object_remove(obj);
	free(obj->key);
	free(obj);
}

void
object_reparent(struct object *obj, struct object *parent)
{
	if (obj->parent != NULL)
		object_remove_child(obj->parent, obj);
	if (parent != NULL)
		object_add_child(parent, obj);
}

struct object *
object_next_child(struct object *obj, struct object *prev)
{
	if (prev == NULL)
		return obj->first_child;
	else
		return prev->next;
}

struct object *
object_next(struct object *prev)
{
	if (prev == NULL)
		return _head;
	else
		return prev->next_all;
}

static void
object_add(struct object *obj)
{
	obj->next_all = _head;
	_head = obj;
}

static void
object_remove(struct object *obj)
{
	struct object			*np, *prev = NULL;

	for (np = _head; np != NULL; prev = np, np = np->next_all)
		if (np == obj) {
			if (obj->parent != NULL)
				object_remove_child(obj->parent, obj);
			if (prev != NULL)
				prev->next_all = np->next_all;
			else if (np == _head)
				_head = np->next_all;
			break;
		}
}

static void
object_remove_child(struct object *obj, struct object *child)
{
	struct object			*np, *prev = NULL;

	for (np = obj->first_child; np != NULL; prev = np, np = np->next)
		if (np == child) {
			if (prev != NULL)
				prev->next = np->next;
			else if (np == obj->first_child)
				obj->first_child = np->next;
			np->parent = NULL;
			break;
		}
}

static void
object_add_child(struct object *obj, struct object *child)
{
	if (child->parent == NULL) {
		child->next = obj->first_child;
		child->parent = obj;
		obj->first_child = child;
	}
}
