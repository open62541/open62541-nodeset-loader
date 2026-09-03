/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H

#include "NodesetLoader/NodesetLoader.h"

#include <stdbool.h>
#include <stddef.h>

struct Alias;
typedef struct Alias Alias;
struct AliasList;
typedef struct AliasList AliasList;
struct NodesetTextBuffer;
typedef struct NodesetTextBuffer NodesetTextBuffer;

typedef enum {
    XML_TOKEN_ELEMENT,
    XML_TOKEN_ATTRIBUTE
} XmlTokenType;

typedef struct {
    XmlTokenType type;
    size_t name;
    size_t content;
    size_t contentLength;
    size_t attributes;
    size_t subtreeEnd;
    size_t start;
    size_t end;
} XmlToken;

typedef struct {
    const XmlToken *tokens;
    size_t size;
    char *text;
} XmlAttributes;

typedef struct {
    NL_Node **nodes;
    size_t size;
    size_t capacity;
} NodeContainer;

typedef struct {
    NodesetTextBuffer *textBuffers;
    AliasList *aliasList;

    NodeContainer nodes[NL_NODECLASS_COUNT];
    NodeContainer allNodes; // gets sorted according to the nodeid
    NodeContainer sortedNodes; // in the order to add to the server

    const NL_FileContext *fc;
    UA_Logger *logger;
} Nodeset;

Nodeset *Nodeset_new(UA_Logger *logger);
bool Nodeset_ownTextBuffer(Nodeset *nodeset, char *data);
void Nodeset_cleanup(Nodeset *nodeset);
bool Nodeset_sort(Nodeset *nodeset);
NL_Node *Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                         const XmlAttributes *attributes);
NL_Reference *Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                                   const XmlAttributes *attributes);
bool Nodeset_newReference_finish(Nodeset *nodeset, NL_Reference *ref,
                                 char *idString);
Alias *Nodeset_newAlias(Nodeset *nodeset, const XmlAttributes *attributes);
bool Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias,
                            char *idString);
bool Nodeset_newNamespaceFinish(Nodeset *nodeset, char *namespaceUri);
bool Nodeset_addDataTypeDefinition(NL_Node *node,
                                   const XmlAttributes *attributes);
bool Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              const XmlAttributes *attributes);
void Nodeset_setDisplayName(NL_Node *node, const XmlAttributes *attributes);
void Nodeset_DisplayNameFinish(NL_Node *node, char *text);
void Nodeset_setDescription(NL_Node *node, const XmlAttributes *attributes);
void Nodeset_DescriptionFinish(NL_Node *node, char *text);
void Nodeset_setInverseName(NL_Node *node, const XmlAttributes *attributes);
void Nodeset_InverseNameFinish(NL_Node *node, char *text);
#endif
