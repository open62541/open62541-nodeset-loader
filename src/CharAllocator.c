#include <CharAllocator.h>
#include <assert.h>
#include <stdlib.h>

struct Region;

struct Region
{
    size_t capacity;
    size_t size;
    struct Region* next;
    char * mem;
};

struct CharArena
{
    size_t initialSize;
    struct Region* current;
};

static struct Region* Region_new(size_t capacity)
{
    struct Region* region = (struct Region *)calloc(1, sizeof(struct Region));
    region->mem = (char *)calloc(capacity, sizeof(char));
    region->capacity = capacity;
    return region;
}

struct CharArena *CharArenaAllocator_new(size_t initialSize)
{
    struct CharArena *arena =
        (struct CharArena *)calloc(1, sizeof(struct CharArena));
    arena->initialSize = initialSize;
    assert(arena);
    arena->current = Region_new(arena->initialSize);
    assert(arena->current);
    return arena;
}

char *CharArenaAllocator_malloc(struct CharArena *arena, size_t size)
{
    if(arena->current->size + size >= arena->current->capacity)
    {
        struct Region* newRegion = Region_new(arena->initialSize);
        if(!newRegion)
        {
            return NULL;
        }
        newRegion->next = arena->current;
        arena->current = newRegion;
    }
    char *mem = arena->current->mem + arena->current->size;
    arena->current->size += size;
    return mem;
}

void CharArenaAllocator_delete(struct CharArena *arena)
{
    struct Region* r = arena->current;
    while(r)
    {
        struct Region* tmp = r->next;
        free(r->mem);
        free(r);
        r=tmp;
    }
    free(arena);
}
