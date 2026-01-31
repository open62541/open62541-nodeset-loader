/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Jan Murzyn
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ServerContext.h"
#include <stdlib.h>

AddNodeContext *
AddNodeContext_new(struct UA_Server *server,
                   NodesetLoader_Logger *logger) {
    // Allocate the context
    AddNodeContext *ctx = (AddNodeContext *)UA_calloc(1, sizeof(AddNodeContext));
    if(!ctx)
        return NULL;
    ctx->server = server;
    ctx->logger = logger;

    // Load initial namespaces from the server
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    size_t idx = 0;
    while(res == UA_STATUSCODE_GOOD) {
        UA_String nsUri = UA_STRING_NULL;;
        res = UA_Server_getNamespaceByIndex(server, idx, &nsUri);
        if(res != UA_STATUSCODE_GOOD)
            continue;
        AddNodeContext_addNamespace(ctx, nsUri);
        UA_String_clear(&nsUri);
        idx++;
    }

    // Get all ReferenceTypes that can point to the parent
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeId = UA_NS0ID(HASCHILD);

    UA_Server_browseRecursive(server, &bd,
                              &ctx->parentRefTypesSize,
                              &ctx->parentRefTypes);

    // Include HasChild itself
    UA_ExpandedNodeId hasChildExp;
    UA_ExpandedNodeId_init(&hasChildExp);
    hasChildExp.nodeId = UA_NS0ID(HASCHILD);
    UA_Array_append((void**)&ctx->parentRefTypes,
                    &ctx->parentRefTypesSize, &hasChildExp,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

    return ctx;
}

void AddNodeContext_delete(AddNodeContext *ctx) {
    UA_NamespaceMapping_clear(&ctx->nsMapping);
    free(ctx);
}

UA_UInt16
AddNodeContext_addNamespace(AddNodeContext *ctx,
                            const UA_String nsUri) {
    // Already in the mapping?
    for(size_t i = 0; i < ctx->nsMapping.namespaceUrisSize; i++) {
        if(UA_String_equal(&nsUri, &ctx->nsMapping.namespaceUris[i]))
            return (UA_UInt16)i;
    }

    // Add to the server
    char namebuf[512];
    memcpy(namebuf, nsUri.data, nsUri.length);
    namebuf[nsUri.length] = 0;
    UA_UInt16 serverIdx = UA_Server_addNamespace(ctx->server, namebuf);
    if(serverIdx == 0)
        return 0;

    // Add to the local mapping
    UA_UInt16 localIdx = (UA_UInt16)ctx->nsMapping.namespaceUrisSize;
    UA_StatusCode res =
        UA_Array_appendCopy((void**)&ctx->nsMapping.namespaceUris,
                            &ctx->nsMapping.namespaceUrisSize,
                            &nsUri, &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD)
        return 0;

    // Add to remote2local
    if(serverIdx > ctx->nsMapping.remote2localSize - 1) {
        // Assert: Nobody else is modifying the server-side namespacearray at the same time
        UA_assert(serverIdx == ctx->nsMapping.remote2localSize);
        res = UA_Array_append((void**)&ctx->nsMapping.remote2local,
                              &ctx->nsMapping.remote2localSize,
                              &localIdx, &UA_TYPES[UA_TYPES_UINT16]);
        (void)res;
    }

    res = UA_Array_append((void**)&ctx->nsMapping.local2remote,
                          &ctx->nsMapping.local2remoteSize,
                          &serverIdx, &UA_TYPES[UA_TYPES_UINT16]);
    (void)res;

    return localIdx;
}
