#ifndef CHARALLOCATOR_H
#define CHARALLOCATOR_H
#include <stddef.h>

struct CharArena;

struct CharArena *CharArenaAllocator_new(size_t initialSize);
char *CharArenaAllocator_malloc(struct CharArena *arena, size_t size);
void CharArenaAllocator_delete(struct CharArena *arena);
void CharArenaAllocator_expand(struct CharArena *arena, size_t size);

#endif
