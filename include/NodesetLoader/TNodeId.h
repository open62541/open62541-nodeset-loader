#ifndef NODESETLOADER_TNODEID_H
#define NODESETLOADER_TNODEID_H
#include "arch.h"
typedef struct
{
    int nsIdx;
    char *id;
} TNodeId;
LOADER_EXPORT int TNodeId_cmp(const TNodeId *id1, const TNodeId *id2);
#endif
