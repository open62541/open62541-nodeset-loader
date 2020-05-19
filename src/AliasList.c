#include "AliasList.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ALIAS 300

struct AliasList
{
    Alias *data;
    size_t size;
};

AliasList *AliasList_new()
{
    struct AliasList *list = (AliasList *)calloc(1, sizeof(*list));
    if(!list)
    {
        return NULL;
    }
    list->data = (Alias *)calloc(MAX_ALIAS, sizeof(Alias));
    if(!list->data)
    {
        free(list);
        return NULL;
    }
    return list;
}

Alias *AliasList_newAlias(AliasList *list, char *name)
{
    if(list->size >= MAX_ALIAS)
    {
        return NULL;
    }
    list->data[list->size].name = name;
    list->data[list->size].id.id = NULL;
    list->data[list->size].id.nsIdx = 0;
    list->size++;
    return &list->data[list->size - 1];
}

const TNodeId *AliasList_getNodeId(const AliasList *list, const char *name)
{
    if(!name)
    {
        return NULL;
    }
    for (Alias* alias = list->data; alias != list->data + list->size; alias++)
    {
        if (!strcmp(name, alias->name))
        {
            return &alias->id;
        }
    }
    return NULL;
}

void AliasList_delete(AliasList *list)
{
    free(list->data);
    free(list);
}
