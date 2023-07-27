/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Jan Murzyn
 */

#include "ServerContext.h"
#include <stdlib.h>

struct ServerContext
{
    UA_Server *server;
    size_t namespaceCnt;
    UA_UInt16 *namespaceIdxMapping;
};

ServerContext *ServerContext_new(UA_Server *server)
{
    ServerContext *serverContext = (ServerContext *)calloc(1, sizeof(ServerContext));
    if (serverContext)
    {
        serverContext->server = server;
        serverContext->namespaceCnt = 0;
        serverContext->namespaceIdxMapping = NULL;
    }

    return serverContext;
}

void ServerContext_delete(ServerContext *serverContext)
{
    free(serverContext->namespaceIdxMapping);
    free(serverContext);
}

UA_Server *ServerContext_getServerObject(const ServerContext *serverContext)
{
    if (!serverContext)
        return NULL;

    return serverContext->server;
}

// Adding server side namespace indices to an array of UA_UInt16.
// Position in the array (minus 1) corresponds to the namespace index in the nodeset file. 
// E.g.
// namespaceIdxMapping [0] = x
//                     [1] = y
//                     [2] = z
//                     ...
// y is a server side namespace index of the namespace that in the nodeset file has index 2. 
void ServerContext_addNamespaceIdx(ServerContext *serverContext, UA_UInt16 serverIdx)
{
    if (!serverContext)
        return;

    void *newNamespaceIdxMapping =
        realloc(serverContext->namespaceIdxMapping, sizeof(UA_UInt16) * (serverContext->namespaceCnt + 1));
    
    if (newNamespaceIdxMapping)
    {
        serverContext->namespaceIdxMapping = (UA_UInt16 *)newNamespaceIdxMapping;
        serverContext->namespaceCnt++;
        serverContext->namespaceIdxMapping[serverContext->namespaceCnt - 1] = serverIdx;
    }
}

// See the comment above ServerContext_addNamespaceIdx
UA_UInt16 ServerContext_translateToServerIdx(const ServerContext *serverContext, UA_UInt16 nodesetIdx)
{
    if (!serverContext)
        return UA_UINT16_MAX;

    if (nodesetIdx == 0)
    {
        // Zero is always 0, no need for translation
        return 0;
    }
    else if ((nodesetIdx > 0) && ((size_t)nodesetIdx <= serverContext->namespaceCnt))
    {
        return serverContext->namespaceIdxMapping[nodesetIdx - 1];
    }
    else
    {
        // Error case. Should it rather be handled by assert(false)?
        return UA_UINT16_MAX;
    }
}
