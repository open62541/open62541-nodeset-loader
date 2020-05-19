/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODE_H
#define NODE_H
#include <NodesetLoader/NodesetLoader.h>

TNode *Node_new(TNodeClass nodeClass);
void Node_delete(TNode *node);

#endif
