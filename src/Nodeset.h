/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H

#include "NodesetLoader/NodesetLoader.h"
#include <ziptree.h>

#include <stdbool.h>
#include <stddef.h>

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

typedef ZIP_HEAD(NodeTree, NL_Node) NodeTree;

typedef struct {
    NL_Node *head;
    NL_Node *tail;
} NodeList;

typedef struct {
    NodesetTextBuffer *textBuffers;
    AliasList *aliasList;

    NodeTree nodeTree;
    NodeList pending[NL_NODECLASS_COUNT];
    NodeList sorted;

    const NL_FileContext *fc;
    UA_Logger *logger;
} Nodeset;

Nodeset *Nodeset_new(UA_Logger *logger);
bool Nodeset_ownTextBuffer(Nodeset *nodeset, char *data);
void Nodeset_cleanup(Nodeset *nodeset);
bool Nodeset_sort(Nodeset *nodeset);
NL_Node *Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                         const XmlAttributes *attributes);
bool Nodeset_addReference(Nodeset *nodeset, NL_Node *node,
                          const XmlAttributes *attributes, char *idString);
bool Nodeset_addAlias(Nodeset *nodeset, const XmlAttributes *attributes,
                      char *idString);
bool Nodeset_addNamespace(Nodeset *nodeset, char *namespaceUri);
bool Nodeset_addDataTypeDefinition(NL_Node *node,
                                   const XmlAttributes *attributes);
bool Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              const XmlAttributes *attributes);
void Nodeset_setLocalizedText(UA_LocalizedText *target,
                              const XmlAttributes *attributes, char *text);
#endif
