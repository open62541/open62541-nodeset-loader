/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Jan Murzyn
 */

#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

#include <open62541/server.h>
#include "NodesetLoader/NodesetLoader.h"
#include "Node.h"

typedef struct {
    UA_Server *server;
    NodeContainer problemNodes;
    UA_NamespaceMapping nsMapping; // From the nodeset (local) to the server (remote)
    UA_Logger *logger;
    UA_DataTypeArray *customTypes; // To be added to the server
} AddNodeContext;

AddNodeContext * AddNodeContext_new(struct UA_Server *server);
void AddNodeContext_delete(AddNodeContext *ctx);

/* Moves the datatype into the addnodecontext */
UA_StatusCode AddNodeContext_addDataType(AddNodeContext *ctx, UA_DataType *t);

// Use AddNodeContext_addNamespaceIdx to sequentially add namespaces as they
// appear in the nodeset file. This adds the namespaces to the server also.
// Returns the local mapping index, not the in-server mapping index.
UA_UInt16 AddNodeContext_addNamespace(AddNodeContext *ctx, const UA_String nsUri);

#endif
