/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODE_H
#define NODE_H

#include "NodesetLoader/NodesetLoader.h"

typedef struct {
    NL_Node **nodes;
    size_t size;
    size_t capacity;
    size_t incrementSize;
    bool owner;
} NodeContainer;

void NodeContainer_init(NodeContainer *container, size_t initialSize, bool owner);
void NodeContainer_clear(NodeContainer *container);
void NodeContainer_delete(NodeContainer *container);
void NodeContainer_add(NodeContainer *container, NL_Node *node);

NL_Node *Node_new(NL_NodeClass nodeClass);
void Node_delete(NL_Node *node);

#endif
