/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "AliasList.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ALIAS 300

struct AliasList
{
    Alias *data;
    size_t size;
};

AliasList *AliasList_new(void)
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
    UA_NodeId_init(&list->data[list->size].id);
    list->size++;
    return &list->data[list->size - 1];
}

const UA_NodeId *
    AliasList_getNodeId(const AliasList *list, const char *name) {
    if(!name)
        return NULL;

    for(Alias* alias = list->data; alias != list->data + list->size; alias++) {
        if (!strcmp(name, alias->name))
            return &alias->id;
    }
    return NULL;
}

void AliasList_delete(AliasList *list)
{
    free(list->data);
    free(list);
}
