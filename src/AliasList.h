#ifndef ALIASLIST_H
#define ALIASLIST_H
#include <NodesetLoader/TNodeId.h>

struct Alias
{
    char *name;
    TNodeId id;
};

typedef struct Alias Alias;

struct AliasList;
typedef struct AliasList AliasList;
AliasList *AliasList_new(void);
Alias *AliasList_newAlias(AliasList *list, char *name);
const TNodeId *AliasList_getNodeId(const AliasList *list, const char *alias);
void AliasList_delete(AliasList *list);

#endif
