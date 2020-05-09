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
