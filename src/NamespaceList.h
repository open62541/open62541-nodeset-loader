#ifndef NAMESPACELIST_H
#define NAMESPACELIST_H
#include <NodesetLoader/NodesetLoader.h>

struct NamespaceList;
typedef struct NamespaceList NamespaceList;

struct Namespace;
typedef struct Namespace Namespace;

struct Namespace
{
    int idx;
    const char *name;
};

NamespaceList *NamespaceList_new(addNamespaceCb cb);
Namespace *NamespaceList_newNamespace(NamespaceList *list, void *userContext,
                                      const char *uri);
void NamespaceList_setUri(NamespaceList *list, Namespace *ns);
void NamespaceList_delete(NamespaceList *list);
const Namespace *NamespaceList_getNamespace(const NamespaceList *list,
                                            int relativeIndex);

#endif
