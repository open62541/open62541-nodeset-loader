/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Jan Murzyn
 */

#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

#include <open62541/server.h>

// ServerContext struct bundles the open62541's UA_Server object
// and a table that maps indices used in the nodeset file to indices used in the server.
struct ServerContext;
typedef struct ServerContext ServerContext;

// ServerContext_new allocates memory that has to released by ServerContext_delete
ServerContext *ServerContext_new(struct UA_Server *server);

// Releases memory allocated by ServerContext_new
void ServerContext_delete(ServerContext *serverContext);

// Gets pointer to the UA_Server object
struct UA_Server *ServerContext_getServerObject(const ServerContext *serverContext);

// Use ServerContext_addNamespaceIdx to sequentially add namespaces as they appear in the
// nodeset file.
void ServerContext_addNamespaceIdx(ServerContext *serverContext, UA_UInt16 serverIdx);

// Translates from an index used in the nodeset file to an index used in the server
UA_UInt16 ServerContext_translateToServerIdx(const ServerContext *serverContext, UA_UInt16 nodesetIdx);

#endif
