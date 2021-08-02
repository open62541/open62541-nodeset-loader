/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H
#include <CharAllocator.h>
#include <NodesetLoader/NodesetLoader.h>
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
struct SortContext;
struct Nodeset
{
    CharArenaAllocator *charArena;
    struct AliasList *aliasList;
    struct NodeContainer *nodes[NL_NODECLASS_COUNT];
    struct NamespaceList *namespaces;
    struct SortContext *sortCtx;
    NL_BiDirectionalReference *hasEncodingRefs;
    NodesetLoader_Logger* logger;
    struct NodeContainer *nodesWithUnknownRefs;
    struct NodeContainer *refTypesWithUnknownRefs;
    NL_ReferenceService* refService;
};

Nodeset *Nodeset_new(NL_addNamespaceCallback nsCallback, NodesetLoader_Logger* logger, NL_ReferenceService* refService);
void Nodeset_cleanup(Nodeset *nodeset);
bool Nodeset_sort(Nodeset *nodeset);
NL_Node *Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                       int attributeSize, const char **attributes);
void Nodeset_newNodeFinish(Nodeset *nodeset, NL_Node *node);
Reference *Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                                int attributeSize, const char **attributes);
void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, NL_Node *node,
                                char *targetId);
struct Alias *Nodeset_newAlias(Nodeset *nodeset, int attributeSize,
                               const char **attribute);
void Nodeset_newAliasFinish(Nodeset *nodeset, struct Alias *alias,
                            char *idString);
void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext,
                                char *namespaceUri);
void Nodeset_addDataTypeDefinition(Nodeset *nodeset, NL_Node *node, int attributeSize,
                              const char **attributes);
void Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node, int attributeSize,
                              const char **attributes);
void Nodeset_setDisplayName(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes);
void Nodeset_DisplayNameFinish(const Nodeset *nodeset, NL_Node *node, char *text);
void Nodeset_setDescription(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes);
void Nodeset_DescriptionFinish(const Nodeset *nodeset, NL_Node *node, char *text);
void Nodeset_setInverseName(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes);
void Nodeset_InverseNameFinish(const Nodeset *nodeset, NL_Node *node, char *text);
const NL_BiDirectionalReference *
Nodeset_getBiDirectionalRefs(const Nodeset *nodeset);
size_t Nodeset_forEachNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                           void *context, NodesetLoader_forEachNode_Func fn);
#endif
