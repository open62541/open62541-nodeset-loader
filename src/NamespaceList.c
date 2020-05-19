#include "NamespaceList.h"
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
    if(!list)
    {
        return NULL;
    }
    list->cb = cb;
    list->size = 1;
    list->data = (Namespace *)calloc(1, sizeof(Namespace));
    if(!list->data)
    {
        free(list);
        return NULL;
    }
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
    if(!list->data)
    {
        return NULL;
    }
    list->data[list->size - 1].name = uri;
    list->data[list->size - 1].idx = globalIdx;
    return &list->data[list->size - 1];
}

const Namespace *NamespaceList_getNamespace(const NamespaceList *list,
                                            int relativeIndex)
{
    if((size_t)relativeIndex >=list->size)
    {
        return NULL;
    }
    return &list->data[relativeIndex];
}
