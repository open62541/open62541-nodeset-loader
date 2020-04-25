/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H
#include <CharAllocator.h>
#include <nodesetLoader/NodesetLoader.h>
#include <stdbool.h>
#include <stddef.h>

struct Nodeset;
typedef struct Nodeset Nodeset;
struct Alias;
struct TParserCtx;
typedef struct TParserCtx TParserCtx;


struct NamespaceList;


struct NodeContainer;
struct AliasList;
struct Nodeset {
    struct CharArena* charArena;
    struct AliasList* aliasList;
    struct NodeContainer *nodes[NODECLASS_COUNT];
    struct NamespaceList* namespaces;
    size_t hierachicalRefsSize;
    TReferenceTypeNode *hierachicalRefs;
};


Nodeset *Nodeset_new(addNamespaceCb nsCallback);
void Nodeset_cleanup(Nodeset *nodeset);
void Nodeset_sort(Nodeset *nodeset);
bool Nodeset_getSortedNodes(Nodeset *nodeset, void *userContext,
                            addNodeCb callback, ValueInterface *valIf);
TNode *Nodeset_newNode(Nodeset *nodeset, TNodeClass nodeClass,
                       int attributeSize, const char **attributes);
void Nodeset_newNodeFinish(Nodeset *nodeset, TNode *node);
Reference *Nodeset_newReference(Nodeset *nodeset, TNode *node,
                                int attributeSize, const char **attributes);
void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, TNode *node,
                                char *targetId);
struct Alias *Nodeset_newAlias(Nodeset *nodeset, int attributeSize,
                        const char **attribute);
void Nodeset_newAliasFinish(Nodeset *nodeset, struct Alias *alias, char *idString);
void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext,
                                char *namespaceUri);
void Nodeset_addDataTypeField(Nodeset *nodeset, TNode *node, int attributeSize,
                              const char **attributes);


#endif
