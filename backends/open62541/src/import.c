/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 *    copyright 2021 (c) jan murzyn
 *    copyright 2025 (c) fraunhofer iosb (author: julius pfrommer)
 */

#include <open62541/server.h>

#include "Nodeset.h"
#include <NodesetLoader/NodesetLoader.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// Sequentially add namespaces as they appear in the nodeset file. This also
// adds them to the server.
UA_StatusCode UA_NodeSetLoaderContext_addNamespace(UA_NodeSetLoaderContext *ctx,
                                                   const UA_String nsUri,
                                                   bool localOnly)
{
    // Get the index / add to the server if required
    if (nsUri.length == SIZE_MAX)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char *name = (char *)UA_malloc(nsUri.length + 1);
    if (!name)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(name, nsUri.data, nsUri.length);
    name[nsUri.length] = 0;
    (void)UA_Server_addNamespace(ctx->server, name);
    UA_free(name);

    size_t serverIdx = 0;
    UA_StatusCode res =
        UA_Server_getNamespaceByName(ctx->server, nsUri, &serverIdx);
    if (res != UA_STATUSCODE_GOOD || serverIdx > UA_UINT16_MAX)
        return res != UA_STATUSCODE_GOOD
                   ? res
                   : UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    UA_UInt16 localIdx = (UA_UInt16)serverIdx;

    // Add to the local mapping
    res = UA_Array_appendCopy((void **)&ctx->namespaceMapping.namespaceUris,
                              &ctx->namespaceMapping.namespaceUrisSize, &nsUri,
                              &UA_TYPES[UA_TYPES_STRING]);
    if (res != UA_STATUSCODE_GOOD)
        return res;

    // Prevent that ns0 is added a second time.
    // This happens when it is named explicitly in the nodeset xml
    // (as the first entry of the list).
    if (localIdx == 0 && ctx->namespaceMapping.remote2localSize > 0)
        return UA_STATUSCODE_GOOD;

    // Add to remote2local only if this comes from the Nodeset xml
    if (!localOnly)
    {
        res = UA_Array_appendCopy((void **)&ctx->namespaceMapping.remote2local,
                                  &ctx->namespaceMapping.remote2localSize,
                                  &localIdx, &UA_TYPES[UA_TYPES_UINT16]);
        if (res != UA_STATUSCODE_GOOD)
            return res;
    }

    // We don't need local2remote since we are only parsing the remote
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeSetLoaderContext_init(UA_NodeSetLoaderContext *ctx,
                                           struct UA_Server *server,
                                           UA_Logger *logger)
{
    memset(ctx, 0, sizeof(UA_NodeSetLoaderContext));
    ctx->server = server;
    ctx->logger = logger;

    // Load initial namespaces from the server.
    // Every nodeset xml implies that ns0 is the first entry.
    // Add it to the remote-namespaces (idx != 0).
    size_t idx = 0;
    for (;;)
    {
        UA_String nsUri = UA_STRING_NULL;
        UA_StatusCode res = UA_Server_getNamespaceByIndex(server, idx, &nsUri);
        if (res == UA_STATUSCODE_BADNOTFOUND)
            break;
        if (res != UA_STATUSCODE_GOOD)
            return res;
        res = UA_NodeSetLoaderContext_addNamespace(ctx, nsUri, idx != 0);
        UA_String_clear(&nsUri);
        if (res != UA_STATUSCODE_GOOD)
            return res;
        idx++;
    }

    // Get all ReferenceTypes that can point to the parent
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeId = UA_NS0ID(HASCHILD);

    UA_StatusCode res = UA_Server_browseRecursive(
        server, &bd, &ctx->parentRefTypesSize, &ctx->parentRefTypes);
    if (res != UA_STATUSCODE_GOOD)
        return res;

    // Include HasChild itself
    UA_ExpandedNodeId hasChildExp;
    UA_ExpandedNodeId_init(&hasChildExp);
    hasChildExp.nodeId = UA_NS0ID(HASCHILD);
    return UA_Array_append((void **)&ctx->parentRefTypes,
                           &ctx->parentRefTypesSize, &hasChildExp,
                           &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

void UA_NodeSetLoaderContext_clear(UA_NodeSetLoaderContext *ctx)
{
    UA_NamespaceMapping_clear(&ctx->namespaceMapping);
    UA_Array_delete(ctx->parentRefTypes, ctx->parentRefTypesSize,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

static inline UA_Boolean isValTrue(const char *s)
{
    if (!s)
        return UA_FALSE;
    if (strcmp(s, "true"))
        return false;
    return true;
}

UA_NodeId UA_NodeSetLoader_getParentId(const UA_NodeSetLoaderContext *ctx,
                                       const NL_Node *node,
                                       UA_NodeId *parentRefId)
{
    for (NL_Reference *ref = node->refs; ref != NULL; ref = ref->next)
    {
        if (ref->isForward)
            continue;
        for (size_t i = 0; i < ctx->parentRefTypesSize; i++)
        {
            if (UA_NodeId_equal(&ref->refType, &ctx->parentRefTypes[i].nodeId))
            {
                if (parentRefId)
                    *parentRefId = ref->refType;
                return ref->target;
            }
        }
    }
    return UA_NODEID_NULL;
}

static UA_NodeId getTypeDefId(const NL_Node *node)
{
    static UA_NodeId typeDefId = {
        0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASTYPEDEFINITION}};
    for (NL_Reference *ref = node->refs; ref != NULL; ref = ref->next)
    {
        if (!ref->isForward)
            continue;
        if (UA_NodeId_equal(&ref->refType, &typeDefId))
            return ref->target;
    }
    return UA_NODEID_NULL;
}

static UA_StatusCode
handleObjectNode(const NL_ObjectNode *node, UA_NodeId *id,
                 const UA_NodeId *parentId, const UA_NodeId *parentReferenceId,
                 const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                 const UA_LocalizedText *description, UA_Server *server)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = *lt;
    oAttr.description = *description;
    oAttr.eventNotifier = (UA_Byte)atoi(node->eventNotifier);

    UA_NodeId typeDefId = getTypeDefId((const NL_Node *)node);

    // addNode_begin is used, otherwise all mandatory childs from type are
    // instantiated
    return UA_Server_addNode_begin(
        server, UA_NODECLASS_OBJECT, *id, *parentId, *parentReferenceId, *qn,
        typeDefId, &oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
}

static UA_StatusCode
handleViewNode(const NL_ViewNode *node, UA_NodeId *id,
               const UA_NodeId *parentId, const UA_NodeId *parentReferenceId,
               const UA_LocalizedText *lt, const UA_QualifiedName *qn,
               const UA_LocalizedText *description, UA_Server *server)
{
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.eventNotifier = (UA_Byte)atoi(node->eventNotifier);
    attr.containsNoLoops = isValTrue(node->containsNoLoops);
    return UA_Server_addViewNode(server, *id, *parentId, *parentReferenceId,
                                 *qn, attr, NULL, NULL);
}

static UA_StatusCode
handleMethodNode(const NL_MethodNode *node, UA_NodeId *id,
                 const UA_NodeId *parentId, const UA_NodeId *parentReferenceId,
                 const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                 const UA_LocalizedText *description, UA_Server *server)
{
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = isValTrue(node->executable);
    attr.userExecutable = isValTrue(node->userExecutable);
    attr.displayName = *lt;
    attr.description = *description;

    return UA_Server_addMethodNode(server, *id, *parentId, *parentReferenceId,
                                   *qn, attr, NULL, 0, NULL, 0, NULL, NULL,
                                   NULL);
}

static UA_StatusCode getArrayDimensions(const char *s, size_t *dimsSize,
                                        UA_UInt32 **dims)
{
    *dimsSize = 0;
    *dims = NULL;
    if (!s || s[0] == 0)
        return UA_STATUSCODE_GOOD;

    size_t count = 1;
    for (const char *pos = s; *pos; pos++)
    {
        if (*pos == ',')
        {
            if (count == SIZE_MAX / sizeof(UA_UInt32))
                return UA_STATUSCODE_BADOUTOFMEMORY;
            count++;
        }
    }

    UA_UInt32 *values = (UA_UInt32 *)UA_malloc(count * sizeof(UA_UInt32));
    if (!values)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    const char *pos = s;
    for (size_t i = 0; i < count; i++)
    {
        errno = 0;
        char *end = NULL;
        unsigned long value = strtoul(pos, &end, 10);
        if (end == pos || errno == ERANGE || value > UA_UINT32_MAX ||
            (*end != ',' && *end != 0))
        {
            UA_free(values);
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        values[i] = (UA_UInt32)value;
        pos = end + (*end == ',');
    }

    *dimsSize = count;
    *dims = values;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode handleVariableNode(
    const NL_VariableNode *node, UA_NodeId *id, const UA_NodeId *parentId,
    const UA_NodeId *parentReferenceId, const UA_LocalizedText *lt,
    const UA_QualifiedName *qn, const UA_LocalizedText *description,
    UA_NodeSetLoaderContext *context)
{
    UA_Server *server = context->server;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = *lt;
    attr.dataType = node->datatype;
    attr.valueRank = atoi(node->valueRank);
    UA_UInt32 *arrDims = NULL;
    UA_StatusCode ret = getArrayDimensions(node->arrayDimensions,
                                           &attr.arrayDimensionsSize, &arrDims);
    if (ret == UA_STATUSCODE_BADOUTOFMEMORY)
        return ret;
    if (ret != UA_STATUSCODE_GOOD)
    {
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Ignoring malformed ArrayDimensions");
        attr.arrayDimensionsSize = 0;
    }
    attr.arrayDimensions = arrDims;
    attr.accessLevel = (UA_Byte)atoi(node->accessLevel);
    attr.userAccessLevel = (UA_Byte)atoi(node->userAccessLevel);
    attr.description = *description;
    attr.historizing = isValTrue(node->historizing);
    attr.minimumSamplingInterval = atof(node->minimumSamplingInterval);

    ret = UA_STATUSCODE_GOOD;
    if (node->value.length > 0)
    {
        UA_DecodeXmlOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
        opts.unwrapped = true;
        opts.customTypes = UA_Server_getDataTypes(server);
        opts.namespaceMapping = &context->namespaceMapping;
        ret = UA_decodeXml(&node->value, &attr.value,
                           &UA_TYPES[UA_TYPES_VARIANT], &opts);
        if (ret != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Failed to parse the value of %N",
                           *id);
    }

    // this case is only needed for the euromap83 comparison, think the nodeset
    // is not valid
    UA_UInt32 arrayDims;
    if (attr.arrayDimensions == NULL && attr.valueRank == 1)
    {
        attr.arrayDimensionsSize = 1;
        arrayDims = 0;
        attr.arrayDimensions = &arrayDims;
    }

    // set arraydimensions of none defined but value is an array
    if (attr.arrayDimensionsSize == 0 && attr.value.arrayLength)
    {
        arrayDims = (UA_UInt32)attr.value.arrayLength;
        attr.arrayDimensions = &arrayDims;
        attr.arrayDimensionsSize = 1;
    }

    UA_NodeId typeDefId = getTypeDefId((const NL_Node *)node);

    // value is copied by open62541
    ret = UA_Server_addNode_begin(
        server, UA_NODECLASS_VARIABLE, *id, *parentId, *parentReferenceId, *qn,
        typeDefId, &attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], NULL, NULL);
    // cannot call addNode finish, otherwise the nodes for e.g. range will be
    // instantiated twice UA_Server_addNode_finish(server, *id);

    UA_Variant_clear(&attr.value);
    if (attr.arrayDimensions && attr.arrayDimensions != &arrayDims)
        UA_free(attr.arrayDimensions);
    return ret;
}

static UA_StatusCode
handleObjectTypeNode(const NL_ObjectTypeNode *node, UA_NodeId *id,
                     const UA_NodeId *parentId,
                     const UA_NodeId *parentReferenceId,
                     const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                     const UA_LocalizedText *description, UA_Server *server)
{
    UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
    oAttr.displayName = *lt;
    oAttr.isAbstract = isValTrue(node->isAbstract);
    oAttr.description = *description;

    return UA_Server_addObjectTypeNode(
        server, *id, *parentId, *parentReferenceId, *qn, oAttr, NULL, NULL);
}

static UA_StatusCode
handleReferenceTypeNode(const NL_ReferenceTypeNode *node, UA_NodeId *id,
                        const UA_NodeId *parentId,
                        const UA_NodeId *parentReferenceId,
                        const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                        const UA_LocalizedText *description, UA_Server *server)
{
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.symmetric = isValTrue(node->symmetric);
    attr.displayName = *lt;
    attr.description = *description;
    attr.inverseName = node->inverseName;
    return UA_Server_addReferenceTypeNode(
        server, *id, *parentId, *parentReferenceId, *qn, attr, NULL, NULL);
}

static UA_StatusCode
handleVariableTypeNode(const NL_VariableTypeNode *node, UA_NodeId *id,
                       const UA_NodeId *parentId,
                       const UA_NodeId *parentReferenceId,
                       const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                       const UA_LocalizedText *description, UA_Server *server)
{
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = *lt;
    attr.dataType = node->datatype;
    attr.description = *description;
    attr.valueRank = atoi(node->valueRank);
    attr.isAbstract = isValTrue(node->isAbstract);
    UA_UInt32 arrayDimensions[1];
    if (attr.valueRank >= 0 && !strcmp(node->arrayDimensions, ""))
    {
        attr.arrayDimensionsSize = 1;
        arrayDimensions[0] = 0;
        attr.arrayDimensions = &arrayDimensions[0];
    }

    return UA_Server_addNode_begin(
        server, UA_NODECLASS_VARIABLETYPE, *id, *parentId, *parentReferenceId,
        *qn, UA_NODEID_NULL, &attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
        NULL, NULL);
}

static UA_StatusCode
handleDataTypeNode(UA_NodeSetLoaderContext *ctx, const NL_DataTypeNode *node,
                   UA_NodeId *id, const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                   const UA_LocalizedText *description)
{
    // Add the UA_DataType to the server. Failure remains lenient: the node may
    // still be useful even if no in-memory encoding description was created.
    (void)UA_NodeSetLoader_addCustomDataType(ctx, node);

    // Add the DataTypeNode
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.isAbstract = isValTrue(node->isAbstract);
    return UA_Server_addDataTypeNode(ctx->server, *id, *parentId,
                                     *parentReferenceId, *qn, attr, NULL, NULL);
}

static bool addNodeFinish(void *contextPtr, NL_Node *node)
{
    UA_NodeSetLoaderContext *context = (UA_NodeSetLoaderContext *)contextPtr;
    UA_StatusCode res = UA_Server_addNode_finish(context->server, node->id);
    if (res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Could not finish node %N (%s)", node->id,
                       UA_StatusCode_name(res));
    return true;
}

static bool addNodeImpl(void *contextPtr, NL_Node *node)
{
    UA_NodeSetLoaderContext *context = (UA_NodeSetLoaderContext *)contextPtr;
    UA_NodeId id = node->id;
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId =
        UA_NodeSetLoader_getParentId(context, node, &parentReferenceId);
    UA_LocalizedText lt = node->displayName;
    UA_QualifiedName qn = node->browseName;
    UA_LocalizedText description = node->description;

    UA_Server *server = context->server;

    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        res = handleObjectNode((const NL_ObjectNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, &description,
                               server);
        break;

    case NODECLASS_METHOD:
        res = handleMethodNode((const NL_MethodNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, &description,
                               server);
        break;

    case NODECLASS_OBJECTTYPE:
        res = handleObjectTypeNode((const NL_ObjectTypeNode *)node, &id,
                                   &parentId, &parentReferenceId, &lt, &qn,
                                   &description, server);
        break;

    case NODECLASS_REFERENCETYPE:
        res = handleReferenceTypeNode((const NL_ReferenceTypeNode *)node, &id,
                                      &parentId, &parentReferenceId, &lt, &qn,
                                      &description, server);
        break;

    case NODECLASS_VARIABLETYPE:
        res = handleVariableTypeNode((const NL_VariableTypeNode *)node, &id,
                                     &parentId, &parentReferenceId, &lt, &qn,
                                     &description, server);
        break;

    case NODECLASS_VARIABLE:
        res = handleVariableNode((const NL_VariableNode *)node, &id, &parentId,
                                 &parentReferenceId, &lt, &qn, &description,
                                 context);
        break;
    case NODECLASS_DATATYPE:
        res = handleDataTypeNode(context, (const NL_DataTypeNode *)node, &id,
                                 &parentId, &parentReferenceId, &lt, &qn,
                                 &description);
        break;
    case NODECLASS_VIEW:
        res =
            handleViewNode((const NL_ViewNode *)node, &id, &parentId,
                           &parentReferenceId, &lt, &qn, &description, server);
        break;
    }

    if (res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Could not add node %N (%s)", node->id,
                       UA_StatusCode_name(res));
    return true;
}

static bool addAllRefs(void *contextPtr, NL_Node *node)
{
    UA_NodeSetLoaderContext *context = (UA_NodeSetLoaderContext *)contextPtr;
    for (NL_Reference *ref = node->refs; ref != NULL; ref = ref->next)
    {
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_StatusCode res = UA_Server_addReference(
            context->server, node->id, ref->refType, target, ref->isForward);
        if (res != UA_STATUSCODE_GOOD &&
            res != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Could not add reference from %N "
                           "to %N (%s)",
                           node->id, ref->target, UA_StatusCode_name(res));
    }
    return true;
}

static void addNodes(UA_NodeSetLoader *loader, UA_NodeSetLoaderContext *anc)
{

    // Add all nodes with their type definition and parent
    (void)UA_NodeSetLoader_forEach(loader, anc, addNodeImpl);

    // Add additional non-hierarchical references
    (void)UA_NodeSetLoader_forEach(loader, anc, addAllRefs);

    // Call AddNode_finish for all nodes
    (void)UA_NodeSetLoader_forEach(loader, anc, addNodeFinish);
}

static UA_StatusCode loadFile(const char *path, UA_ByteString *xml)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_StatusCode status = UA_STATUSCODE_BADINTERNALERROR;
    if (fseek(file, 0, SEEK_END) != 0)
        goto cleanup;
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0)
        goto cleanup;
    size_t size = (size_t)length;
    if ((long)size != length)
    {
        status = UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        goto cleanup;
    }

    status = UA_ByteString_allocBuffer(xml, size);
    if (status != UA_STATUSCODE_GOOD)
        goto cleanup;
    if (fread(xml->data, 1, size, file) != size)
    {
        UA_ByteString_clear(xml);
        status = UA_STATUSCODE_BADINTERNALERROR;
    }

cleanup:
    fclose(file);
    return status;
}

UA_StatusCode UA_Server_loadNodeset(UA_Server *server, const char *path)
{
    if (!server || !path)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Logger *logger = config->logging;

    UA_ByteString xml = UA_BYTESTRING_NULL;
    UA_StatusCode res = loadFile(path, &xml);
    if (res != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: Could not read %s (%s)", path,
                     UA_StatusCode_name(res));
        return res;
    }

    UA_NodeSetLoaderContext ctx;
    res = UA_NodeSetLoaderContext_init(&ctx, server, logger);
    if (res != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: Could not initialize import (%s)",
                     UA_StatusCode_name(res));
        UA_NodeSetLoaderContext_clear(&ctx);
        UA_ByteString_clear(&xml);
        return res;
    }
    UA_NodeSetLoader *loader = UA_NodeSetLoader_new(&ctx);
    if (!loader)
    {
        UA_NodeSetLoaderContext_clear(&ctx);
        UA_ByteString_clear(&xml);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SERVER,
                 "NodeSetLoader: Start import nodeset: %s", path);
    res = UA_NodeSetLoader_import(loader, &xml);
    if (res == UA_STATUSCODE_GOOD && !UA_NodeSetLoader_sort(loader))
        res = UA_STATUSCODE_BADDECODINGERROR;
    if (res == UA_STATUSCODE_GOOD)
        addNodes(loader, &ctx);
    if (res != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: Importing the nodeset failed (%s)",
                     UA_StatusCode_name(res));
    UA_NodeSetLoader_delete(loader);
    UA_NodeSetLoaderContext_clear(&ctx);
    UA_ByteString_clear(&xml);
    return res;
}
