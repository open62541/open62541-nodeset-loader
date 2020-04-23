/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H
#include <charAllocator.h>
#include <nodesetLoader/nodesetLoader.h>
#include <stdbool.h>
#include <stddef.h>

struct Nodeset;
typedef struct Nodeset Nodeset;

typedef struct {
    char *name;
    TNodeId id;
} Alias;

typedef struct {
    size_t cnt;
    TNode **nodes;
} NodeContainer;

struct TParserCtx;
typedef struct TParserCtx TParserCtx;

struct TNamespace;
typedef struct TNamespace TNamespace;

struct TNamespace {
    size_t idx;
    char *name;
};

typedef struct {
    size_t size;
    TNamespace *ns;
    addNamespaceCb cb;
} TNamespaceTable;

struct Nodeset {
    Reference **countedRefs;
    struct CharArena* charArena;
    Alias **aliasArray;
    NodeContainer *nodes[NODECLASS_COUNT];
    size_t aliasSize;
    size_t refsSize;
    TNamespaceTable *namespaceTable;
    size_t hierachicalRefsSize;
    TReferenceTypeNode *hierachicalRefs;
};

TNodeId extractNodedId(const TNamespace *namespaces, char *s);
TBrowseName extractBrowseName(const TNamespace *namespaces, char *s);
TNodeId translateNodeId(const TNamespace *namespaces, TNodeId id);
TBrowseName translateBrowseName(const TNamespace *namespaces, TBrowseName id);
Nodeset* Nodeset_new(addNamespaceCb nsCallback);
void Nodeset_cleanup(Nodeset* nodeset);
void Nodeset_sort(Nodeset *nodeset);
bool Nodeset_getSortedNodes(Nodeset *nodeset, void *userContext, addNodeCb callback, ValueInterface* valIf);
TNode *Nodeset_newNode(Nodeset *nodeset, TNodeClass nodeClass, int attributeSize,
                       const char **attributes);
void Nodeset_newNodeFinish(Nodeset *nodeset, TNode *node);
Reference *Nodeset_newReference(Nodeset *nodeset, TNode *node, int attributeSize,
                                const char **attributes);
void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, TNode *node,
                                char *targetId);
Alias *Nodeset_newAlias(Nodeset *nodeset, int attributeSize, const char **attribute);
void Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString);
TNamespace *Nodeset_newNamespace(Nodeset *nodeset);
void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext, char *namespaceUri);

#endif
