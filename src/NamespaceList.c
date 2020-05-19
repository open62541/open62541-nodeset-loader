/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "NamespaceList.h"
#include <assert.h>
#include <stdlib.h>

struct NamespaceList
{
    size_t size;
    Namespace *data;
    addNamespaceCb cb;
};

NamespaceList *NamespaceList_new(addNamespaceCb cb)
{
    NamespaceList *list = (NamespaceList *)calloc(1, sizeof(NamespaceList));
    assert(list);
    list->cb = cb;
    list->size = 1;
    list->data = (Namespace *)calloc(1, sizeof(Namespace));
    assert(list->data);
    list->data[0].name = "http://opcfoundation.org/UA/";
    list->data[0].idx = 0;
    return list;
}

void NamespaceList_delete(NamespaceList *list)
{
    free(list->data);
    free(list);
}

Namespace *NamespaceList_newNamespace(NamespaceList *list, void *userContext,
                                      const char *uri)
{
    // ask backend to create/get overall namespaceIndex
    int globalIdx = list->cb(userContext, uri);
    list->size++;
    list->data =
        (Namespace *)realloc(list->data, sizeof(Namespace) * list->size);
    assert(list->data);
    list->data[list->size - 1].name = uri;
    list->data[list->size - 1].idx = globalIdx;
    return &list->data[list->size - 1];
}

const Namespace *NamespaceList_getNamespace(const NamespaceList *list,
                                            int relativeIndex)
{
    assert(relativeIndex < (int)list->size);
    return &list->data[relativeIndex];
}
