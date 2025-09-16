/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H

#include "NodesetLoader/NodesetLoader.h"
#include "CharAllocator.h"
#include "Node.h"

#include <stdbool.h>
#include <stddef.h>

struct TParserCtx;
typedef struct TParserCtx TParserCtx;

struct Nodeset;
typedef struct Nodeset Nodeset;
struct Alias;
struct AliasList;

struct Nodeset {
    CharArenaAllocator *charArena;
    struct AliasList *aliasList;

    NodeContainer nodes[NL_NODECLASS_COUNT];
    NodeContainer allNodes; // gets sorted according to the nodeid
    NodeContainer sortedNodes; // in the order to add to the server

    NL_FileContext *fc;
    size_t localNamespaceUrisSize;
    UA_String *localNamespaceUris;

    NodesetLoader_Logger* logger;
};

Nodeset *Nodeset_new(NL_addNamespaceCallback nsCallback,
                     NodesetLoader_Logger* logger);
void Nodeset_cleanup(Nodeset *nodeset);
bool Nodeset_sort(Nodeset *nodeset);
NL_Node *Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                         size_t attributeSize, const char **attributes);
NL_Reference *Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                                   size_t attributeSize, const char **attributes);
void Nodeset_newReference_finish(Nodeset *nodeset, NL_Reference *ref,
                                 char *idString);
struct Alias *Nodeset_newAlias(Nodeset *nodeset, size_t attributeSize,
                               const char **attribute);
void Nodeset_newAliasFinish(Nodeset *nodeset, struct Alias *alias,
                            char *idString);
void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext,
                                char *namespaceUri);
void Nodeset_addDataTypeDefinition(Nodeset *nodeset, NL_Node *node,
                                   size_t attributeSize, const char **attributes);
void Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              size_t attributeSize, const char **attributes);
void Nodeset_setDisplayName(Nodeset *nodeset, NL_Node *node,
                            size_t attributeSize, const char **attributes);
void Nodeset_DisplayNameFinish(const Nodeset *nodeset, NL_Node *node, char *text);
void Nodeset_setDescription(Nodeset *nodeset, NL_Node *node, size_t attributeSize,
                            const char **attributes);
void Nodeset_DescriptionFinish(const Nodeset *nodeset, NL_Node *node, char *text);
void Nodeset_setInverseName(Nodeset *nodeset, NL_Node *node, size_t attributeSize,
                            const char **attributes);
void Nodeset_InverseNameFinish(const Nodeset *nodeset, NL_Node *node, char *text);
bool Nodeset_forEachNode(Nodeset *nodeset, void *context,
                         NodesetLoader_forEachNode_Func fn);

#endif
