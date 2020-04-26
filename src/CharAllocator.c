#include <CharAllocator.h>
#include <assert.h>
#include <stdlib.h>

struct CharArena
{
    size_t initialSize;
    size_t currentSize;
    char *mem;
};

struct CharArena *CharArenaAllocator_new(size_t initialSize)
{
    struct CharArena *arena =
        (struct CharArena *)calloc(1, sizeof(struct CharArena));
    arena->initialSize = initialSize;
    assert(arena);
    arena->mem = (char *)calloc(initialSize, sizeof(char));
    assert(arena->mem);
    return arena;
}

char *CharArenaAllocator_malloc(struct CharArena *arena, size_t size)
{
    assert(size + arena->currentSize <= arena->initialSize);
    char *mem = arena->mem + arena->currentSize;
    arena->currentSize += size;
    return mem;
}

void CharArenaAllocator_expand(struct CharArena *arena, size_t size)
{
    assert(size + arena->currentSize <= arena->initialSize);
    arena->currentSize += size;
}

void CharArenaAllocator_delete(struct CharArena *arena)
{
    free(arena->mem);
    free(arena);
}
