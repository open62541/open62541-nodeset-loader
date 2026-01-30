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

bool NodeContainer_init(NodeContainer *container, size_t initialSize);
void NodeContainer_clear(NodeContainer *container);
bool NodeContainer_add(NodeContainer *container, NL_Node *node);
void NodeContainer_remove(NodeContainer *container, size_t index);

NL_Node *Node_new(NL_NodeClass nodeClass);
void Node_delete(NL_Node *node);

void DataTypeNode_clear(NL_DataTypeNode *node);
NL_DataTypeDefinition *DataTypeDefinition_new(NL_DataTypeNode *node);
NL_DataTypeDefinitionField *DataTypeNode_addDefinitionField(NL_DataTypeDefinition *def);

#endif
