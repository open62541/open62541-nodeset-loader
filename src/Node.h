/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODE_H
#define NODE_H

#include "NodesetLoader/NodesetLoader.h"
#include "Nodeset.h"

struct NodeContainer {
    NL_Node **nodes;
    size_t size;
    size_t capacity;
    size_t incrementSize;
    bool owner;
};
typedef struct NodeContainer NodeContainer;

NodeContainer *NodeContainer_new(size_t initialSize, bool owner);
void NodeContainer_delete(NodeContainer *container);
void NodeContainer_add(NodeContainer *container, NL_Node *node);

NL_Node *Node_new(NL_NodeClass nodeClass);
void Node_delete(NL_Node *node);

void DataTypeNode_clear(NL_DataTypeNode *node);
NL_DataTypeDefinition *DataTypeDefinition_new(NL_DataTypeNode *node);
NL_DataTypeDefinitionField *DataTypeNode_addDefinitionField(NL_DataTypeDefinition *def);

#endif
