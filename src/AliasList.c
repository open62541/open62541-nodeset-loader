#include "AliasList.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_ALIAS 100

struct AliasList
{
    Alias* data;
    size_t size;
};

AliasList *AliasList_new()
{
    struct AliasList* list = (AliasList*)calloc(1, sizeof(*list));
    assert(list);
    list->data = (Alias*)calloc(MAX_ALIAS, sizeof(Alias));
    assert(list->data);
    return list;
}

Alias* AliasList_newAlias(AliasList* list, char* name)
{
    list->data[list->size].name = name;
    list->data[list->size].id.id = NULL;
    list->data[list->size].id.nsIdx = 0;
    list->size++;
    return &list->data[list->size-1];
}

const TNodeId* AliasList_getNodeId(const AliasList* list, const char *name)
{
    for (size_t cnt = 0; cnt < list->size; cnt++)
    {
        if (!strcmp(name, list->data[cnt].name))
        {
            return &list->data[cnt].id;
        }
    }
    return NULL;
}

void AliasList_delete(AliasList *list)
{
    //for (size_t cnt = 0; cnt < n->aliasSize; cnt++)
    //{
    //    free(n->aliasArray[cnt]);
    //}
    free(list->data);
    free(list);
}
