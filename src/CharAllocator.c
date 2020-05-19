/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <CharAllocator.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Region;

struct Region
{
    size_t capacity;
    size_t size;
    struct Region *next;
    char *mem;
    char *userPtr;
    size_t userSize;
};

struct CharArenaAllocator
{
    size_t initialSize;
    struct Region *current;
};

static struct Region *Region_new(size_t capacity)
{
    struct Region *region = (struct Region *)calloc(1, sizeof(struct Region));
    region->mem = (char *)calloc(capacity, sizeof(char));
    region->capacity = capacity;
    region->userPtr = region->mem;
    return region;
}

CharArenaAllocator *CharArenaAllocator_new(size_t initialSize)
{
    CharArenaAllocator *arena =
        (CharArenaAllocator *)calloc(1, sizeof(CharArenaAllocator));
    arena->initialSize = initialSize;
    assert(arena);
    arena->current = Region_new(arena->initialSize);
    assert(arena->current);
    return arena;
}

static size_t getRegionSize(size_t requested, size_t initialArenaSize)
{
    size_t regionSize = initialArenaSize;
    if (requested > initialArenaSize)
    {
        regionSize = requested * 2;
    }
    return regionSize;
}

char *CharArenaAllocator_malloc(CharArenaAllocator *arena, size_t size)
{
    if ((arena->current->size + size) > arena->current->capacity)
    {        
        struct Region *newRegion = Region_new(getRegionSize(size, arena->initialSize));
        if (!newRegion)
        {
            return NULL;
        }
        newRegion->next = arena->current;
        arena->current = newRegion;
    }
    arena->current->userPtr = arena->current->mem + arena->current->size;
    arena->current->size += size;
    arena->current->userSize = size;
    return arena->current->userPtr;
}

char *CharArenaAllocator_realloc(CharArenaAllocator *arena, size_t size)
{
    if ((arena->current->size + size) > arena->current->capacity)
    {
        // we also have to consider the size we have to transfer
        struct Region *newRegion =
            Region_new(getRegionSize(size + arena->current->userSize*2, arena->initialSize));
        if (!newRegion)
        {
            return NULL;
        }
        //we have to copy over the old stuff
        memcpy(newRegion->userPtr, arena->current->userPtr, arena->current->userSize);
        newRegion->userSize = arena->current->userSize;
        newRegion->next = arena->current;
        newRegion->size = newRegion->userSize;
        arena->current = newRegion;
    }
    arena->current->userSize += size;
    arena->current->size += size;
    return arena->current->userPtr;
}

void CharArenaAllocator_delete(CharArenaAllocator *arena)
{
    struct Region *r = arena->current;
    while (r)
    {
        struct Region *tmp = r->next;
        free(r->mem);
        free(r);
        r = tmp;
    }
    free(arena);
}
