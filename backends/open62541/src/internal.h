/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2016 (c) o6 Automation Gmbh (Author: Julius Pfrommer)
 */

#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#include <open62541/server.h>
#include "NodesetLoader/NodesetLoader.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    UA_Server *server;
    UA_NamespaceMapping nsMapping; // From the nodeset (local) to the server (remote)
    NodesetLoader_Logger *logger;

    // ReferenceTypes that can point to a parent.
    // Inherited from HasChild.
    size_t parentRefTypesSize;
    UA_ExpandedNodeId *parentRefTypes;
} AddNodeContext;

UA_NodeId
getParentId(const AddNodeContext *ctx, const NL_Node *node, UA_NodeId *parentRefId);

void
addCustomDataType(AddNodeContext *ctx, const NL_DataTypeNode *node);

#ifdef __cplusplus
}
#endif
#endif

