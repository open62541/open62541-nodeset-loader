/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef CHARALLOCATOR_H
#define CHARALLOCATOR_H
#include <stddef.h>

struct CharArenaAllocator;
typedef struct CharArenaAllocator CharArenaAllocator;

CharArenaAllocator *CharArenaAllocator_new(size_t initialSize);
char *CharArenaAllocator_malloc(struct CharArenaAllocator *arena, size_t size);
char *CharArenaAllocator_realloc(struct CharArenaAllocator *arena, size_t size);
void CharArenaAllocator_delete(struct CharArenaAllocator *arena);

#endif
