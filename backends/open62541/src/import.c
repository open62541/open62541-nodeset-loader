/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 *    copyright 2021 (c) jan murzyn
 *    copyright 2025 (c) fraunhofer iosb (author: julius pfrommer)
 */

#include <open62541/server.h>

#include <NodesetLoader/backendOpen62541.h>
#include "internal.h"

#include <errno.h>
#include <stdlib.h>

// Use AddNodeContext_addNamespaceIdx to sequentially add namespaces as they
// appear in the nodeset file. This adds the namespaces to the server also.
// Returns the local mapping index, not the in-server mapping index.
static UA_StatusCode
AddNodeContext_addNamespace(AddNodeContext *ctx, const UA_String nsUri,
                            bool localOnly) {
    // Get the index / add to the server if required
    if(nsUri.length == SIZE_MAX)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char *name = (char*)UA_malloc(nsUri.length + 1);
    if(!name)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(name, nsUri.data, nsUri.length);
    name[nsUri.length] = 0;
    (void)UA_Server_addNamespace(ctx->server, name);
    UA_free(name);

    size_t serverIdx = 0;
    UA_StatusCode res =
        UA_Server_getNamespaceByName(ctx->server, nsUri, &serverIdx);
    if(res != UA_STATUSCODE_GOOD || serverIdx > UA_UINT16_MAX)
        return res != UA_STATUSCODE_GOOD ? res :
            UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    UA_UInt16 localIdx = (UA_UInt16)serverIdx;

    // Add to the local mapping
    res = UA_Array_appendCopy((void**)&ctx->nsMapping.namespaceUris,
                              &ctx->nsMapping.namespaceUrisSize,
                              &nsUri, &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    // Prevent that ns0 is added a second time.
    // This happens when it is named explicitly in the nodeset xml
    // (as the first entry of the list).
    if(localIdx == 0 && ctx->nsMapping.remote2localSize > 0)
        return UA_STATUSCODE_GOOD;

    // Add to remote2local only if this comes from the Nodeset xml
    if(!localOnly) {
        res = UA_Array_appendCopy((void**)&ctx->nsMapping.remote2local,
                                  &ctx->nsMapping.remote2localSize,
                                  &localIdx, &UA_TYPES[UA_TYPES_UINT16]);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    // We don't need local2remote since we are only parsing the remote
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
AddNodeContext_init(AddNodeContext *ctx,
                    struct UA_Server *server,
                    UA_Logger *logger) {
    memset(ctx, 0, sizeof(AddNodeContext));
    ctx->server = server;
    ctx->logger = logger;

    // Load initial namespaces from the server.
    // Every nodeset xml implies that ns0 is the first entry.
    // Add it to the remote-namespaces (idx != 0).
    size_t idx = 0;
    for(;;) {
        UA_String nsUri = UA_STRING_NULL;
        UA_StatusCode res =
            UA_Server_getNamespaceByIndex(server, idx, &nsUri);
        if(res == UA_STATUSCODE_BADNOTFOUND)
            break;
        if(res != UA_STATUSCODE_GOOD)
            return res;
        res = AddNodeContext_addNamespace(ctx, nsUri, idx != 0);
        UA_String_clear(&nsUri);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        idx++;
    }
    
    // Get all ReferenceTypes that can point to the parent
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeId = UA_NS0ID(HASCHILD);

    UA_StatusCode res = UA_Server_browseRecursive(server, &bd,
                                                  &ctx->parentRefTypesSize,
                                                  &ctx->parentRefTypes);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    // Include HasChild itself
    UA_ExpandedNodeId hasChildExp;
    UA_ExpandedNodeId_init(&hasChildExp);
    hasChildExp.nodeId = UA_NS0ID(HASCHILD);
    return UA_Array_append((void**)&ctx->parentRefTypes,
                           &ctx->parentRefTypesSize, &hasChildExp,
                           &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

static void
AddNodeContext_clear(AddNodeContext *ctx) {
    UA_NamespaceMapping_clear(&ctx->nsMapping);
    UA_Array_delete(ctx->parentRefTypes, ctx->parentRefTypesSize,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

static inline UA_Boolean isValTrue(const char *s) {
    if(!s)
        return UA_FALSE;
    if(strcmp(s, "true"))
        return false;
    return true;
}

UA_NodeId
getParentId(const AddNodeContext *ctx, const NL_Node *node, UA_NodeId *parentRefId) {
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        if(ref->isForward)
            continue;
        for(size_t i = 0; i < ctx->parentRefTypesSize; i++) {
            if(UA_NodeId_equal(&ref->refType, &ctx->parentRefTypes[i].nodeId)) {
                if(parentRefId)
                    *parentRefId = ref->refType;
                return ref->target;
            }
        }
    }
    return UA_NODEID_NULL;
}

static UA_NodeId
getTypeDefId(const NL_Node *node) {
    static UA_NodeId typeDefId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASTYPEDEFINITION}};
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        if(!ref->isForward)
            continue;
        if(UA_NodeId_equal(&ref->refType, &typeDefId))
            return ref->target;
    }
    return UA_NODEID_NULL;
}

static UA_StatusCode
handleObjectNode(const NL_ObjectNode *node, UA_NodeId *id,
                 const UA_NodeId *parentId, const UA_NodeId *parentReferenceId,
                 const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                 const UA_LocalizedText *description, UA_Server *server) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = *lt;
    oAttr.description = *description;
    oAttr.eventNotifier = (UA_Byte)atoi(node->eventNotifier);

    UA_NodeId typeDefId = getTypeDefId((const NL_Node*)node);

    // addNode_begin is used, otherwise all mandatory childs from type are
    // instantiated
    return UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, *id, *parentId,
                            *parentReferenceId, *qn, typeDefId, &oAttr,
                            &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            NULL, NULL);
}

static UA_StatusCode
handleViewNode(const NL_ViewNode *node, UA_NodeId *id, const UA_NodeId *parentId,
               const UA_NodeId *parentReferenceId, const UA_LocalizedText *lt,
               const UA_QualifiedName *qn, const UA_LocalizedText *description,
               UA_Server *server) {
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
                 const UA_LocalizedText *description, UA_Server *server) {
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = isValTrue(node->executable);
    attr.userExecutable = isValTrue(node->userExecutable);
    attr.displayName = *lt;
    attr.description = *description;

    return UA_Server_addMethodNode(server, *id, *parentId, *parentReferenceId,
                                   *qn, attr, NULL, 0, NULL, 0, NULL,
                                   NULL, NULL);
}

static UA_StatusCode
getArrayDimensions(const char *s, size_t *dimsSize, UA_UInt32 **dims) {
    *dimsSize = 0;
    *dims = NULL;
    if(!s || s[0] == 0)
        return UA_STATUSCODE_GOOD;

    size_t count = 1;
    for(const char *pos = s; *pos; pos++) {
        if(*pos == ',') {
            if(count == SIZE_MAX / sizeof(UA_UInt32))
                return UA_STATUSCODE_BADOUTOFMEMORY;
            count++;
        }
    }

    UA_UInt32 *values = (UA_UInt32*)UA_malloc(count * sizeof(UA_UInt32));
    if(!values)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    const char *pos = s;
    for(size_t i = 0; i < count; i++) {
        errno = 0;
        char *end = NULL;
        unsigned long value = strtoul(pos, &end, 10);
        if(end == pos || errno == ERANGE || value > UA_UINT32_MAX ||
           (*end != ',' && *end != 0)) {
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

static UA_StatusCode
handleVariableNode(const NL_VariableNode *node, UA_NodeId *id,
                   const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt,
                   const UA_QualifiedName *qn,
                   const UA_LocalizedText *description,
                   AddNodeContext *context) {
    UA_Server *server = context->server;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = *lt;
    attr.dataType = node->datatype;
    attr.valueRank = atoi(node->valueRank);
    UA_UInt32 *arrDims = NULL;
    UA_StatusCode ret = getArrayDimensions(node->arrayDimensions,
                                           &attr.arrayDimensionsSize,
                                           &arrDims);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        return ret;
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodesetLoader: Ignoring malformed ArrayDimensions");
        attr.arrayDimensionsSize = 0;
    }
    attr.arrayDimensions = arrDims;
    attr.accessLevel = (UA_Byte)atoi(node->accessLevel);
    attr.userAccessLevel = (UA_Byte)atoi(node->userAccessLevel);
    attr.description = *description;
    attr.historizing = isValTrue(node->historizing);
    attr.minimumSamplingInterval = atof(node->minimumSamplingInterval);

    char buf[128];
    memset(buf, 0, 128);
    UA_String idBuf = {128, (UA_Byte*)buf};
    UA_NodeId_print(id, &idBuf);

    ret = UA_STATUSCODE_GOOD;
    if(node->value.length > 0) {
        UA_DecodeXmlOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
        opts.unwrapped = true;
        opts.customTypes = UA_Server_getDataTypes(server);
        opts.namespaceMapping = &context->nsMapping;
        ret = UA_decodeXml(&node->value, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);
        if(ret != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                           "NodesetLoader: Failed to parse the value of %s", buf);
    }

    // this case is only needed for the euromap83 comparison, think the nodeset
    // is not valid
    UA_UInt32 arrayDims;
    if (attr.arrayDimensions == NULL && attr.valueRank == 1) {
        attr.arrayDimensionsSize = 1;
        arrayDims = 0;
        attr.arrayDimensions = &arrayDims;
    }

    // set arraydimensions of none defined but value is an array
    if (attr.arrayDimensionsSize == 0 && attr.value.arrayLength) {
        arrayDims = (UA_UInt32)attr.value.arrayLength;
        attr.arrayDimensions = &arrayDims;
        attr.arrayDimensionsSize = 1;
    }

    UA_NodeId typeDefId = getTypeDefId((const NL_Node*)node);

    //value is copied by open62541
    ret = UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, *id, *parentId,
                                  *parentReferenceId, *qn, typeDefId, &attr,
                                  &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                                  NULL, NULL);
    //cannot call addNode finish, otherwise the nodes for e.g. range will be instantiated twice
    //UA_Server_addNode_finish(server, *id);

    UA_Variant_clear(&attr.value);
    if(attr.arrayDimensions && attr.arrayDimensions != &arrayDims)
        UA_free(attr.arrayDimensions);
    return ret;
}

static UA_StatusCode
handleObjectTypeNode(const NL_ObjectTypeNode *node, UA_NodeId *id,
                     const UA_NodeId *parentId,
                     const UA_NodeId *parentReferenceId,
                     const UA_LocalizedText *lt,
                     const UA_QualifiedName *qn,
                     const UA_LocalizedText *description,
                     UA_Server *server) {
    UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
    oAttr.displayName = *lt;
    oAttr.isAbstract = isValTrue(node->isAbstract);
    oAttr.description = *description;

    return UA_Server_addObjectTypeNode(server, *id, *parentId,
                                       *parentReferenceId, *qn,
                                       oAttr, NULL, NULL);
}

static UA_StatusCode
handleReferenceTypeNode(const NL_ReferenceTypeNode *node,
                        UA_NodeId *id, const UA_NodeId *parentId,
                        const UA_NodeId *parentReferenceId,
                        const UA_LocalizedText *lt,
                        const UA_QualifiedName *qn,
                        const UA_LocalizedText *description,
                        UA_Server *server) {
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.symmetric = isValTrue(node->symmetric);
    attr.displayName = *lt;
    attr.description = *description;
    attr.inverseName = node->inverseName;
    return UA_Server_addReferenceTypeNode(server, *id, *parentId, *parentReferenceId,
                                   *qn, attr, NULL, NULL);
}

static UA_StatusCode
handleVariableTypeNode(const NL_VariableTypeNode *node, UA_NodeId *id,
                       const UA_NodeId *parentId,
                       const UA_NodeId *parentReferenceId,
                       const UA_LocalizedText *lt,
                       const UA_QualifiedName *qn,
                       const UA_LocalizedText *description,
                       UA_Server *server) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = *lt;
    attr.dataType = node->datatype;
    attr.description = *description;
    attr.valueRank = atoi(node->valueRank);
    attr.isAbstract = isValTrue(node->isAbstract);
    UA_UInt32 arrayDimensions[1];
    if (attr.valueRank >= 0 && !strcmp(node->arrayDimensions, "")) {
        attr.arrayDimensionsSize = 1;
        arrayDimensions[0] = 0;
        attr.arrayDimensions = &arrayDimensions[0];
    }

   return UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLETYPE,
                                  *id, *parentId, *parentReferenceId, *qn,
                                  UA_NODEID_NULL, &attr,
                                  &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                                  NULL, NULL);
}

static UA_StatusCode
handleDataTypeNode(AddNodeContext *ctx,
                   const NL_DataTypeNode *node, UA_NodeId *id,
                   const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt,
                   const UA_QualifiedName *qn,
                   const UA_LocalizedText *description) {
    // Add the UA_DataType to the server. Failure remains lenient: the node may
    // still be useful even if no in-memory encoding description was created.
    (void)addCustomDataType(ctx, node);

    // Add the DataTypeNode
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.isAbstract = isValTrue(node->isAbstract);
    return UA_Server_addDataTypeNode(ctx->server, *id, *parentId,
                                     *parentReferenceId, *qn,
                                     attr, NULL, NULL);
}

static bool
addNodeFinish(void *contextPtr, NL_Node *node) {
    AddNodeContext *context = (AddNodeContext*)contextPtr;
    UA_StatusCode res =
        UA_Server_addNode_finish(context->server, node->id);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodesetLoader: Could not finish node %N (%s)",
                       node->id, UA_StatusCode_name(res));
    return true;
}

static bool
addNodeImpl(void *contextPtr, NL_Node *node) {
    AddNodeContext *context = (AddNodeContext*)contextPtr;
    UA_NodeId id = node->id;
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(context, node, &parentReferenceId);
    UA_LocalizedText lt = node->displayName;
    UA_QualifiedName qn = node->browseName;
    UA_LocalizedText description = node->description;

    UA_Server *server = context->server;

    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    switch (node->nodeClass) {
    case NODECLASS_OBJECT:
        res = handleObjectNode((const NL_ObjectNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, &description, server);
        break;

    case NODECLASS_METHOD:
        res = handleMethodNode((const NL_MethodNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, &description, server);
        break;

    case NODECLASS_OBJECTTYPE:
        res = handleObjectTypeNode((const NL_ObjectTypeNode *)node, &id, &parentId,
                                   &parentReferenceId, &lt, &qn, &description, server);
        break;

    case NODECLASS_REFERENCETYPE:
        res = handleReferenceTypeNode((const NL_ReferenceTypeNode *)node, &id, &parentId,
                                      &parentReferenceId, &lt, &qn, &description, server);
        break;

    case NODECLASS_VARIABLETYPE:
        res = handleVariableTypeNode((const NL_VariableTypeNode *)node, &id, &parentId,
                                     &parentReferenceId, &lt, &qn, &description, server);
        break;

    case NODECLASS_VARIABLE:
        res = handleVariableNode((const NL_VariableNode *)node, &id, &parentId,
                                 &parentReferenceId, &lt, &qn, &description, context);
        break;
    case NODECLASS_DATATYPE:
        res = handleDataTypeNode(context, (const NL_DataTypeNode *)node,
                                 &id, &parentId, &parentReferenceId,
                                 &lt, &qn, &description);
        break;
    case NODECLASS_VIEW:
        res = handleViewNode((const NL_ViewNode *)node, &id, &parentId,
                             &parentReferenceId, &lt, &qn, &description, server);
        break;
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                       "NodesetLoader: Could not add node %N (%s)",
                       node->id, UA_StatusCode_name(res));
    return true;
}

static bool
NodesetLoader_BackendOpen62541_addNamespace(void *userContext,
                                            size_t localNamespaceUrisSize,
                                            UA_String *localNamespaceUris,
                                            UA_NamespaceMapping *nsMapping) {
    (void)nsMapping;
    AddNodeContext *ctx = (AddNodeContext*)userContext;
    for(size_t i = 0; i < localNamespaceUrisSize; i++) {
        UA_StatusCode res =
            AddNodeContext_addNamespace(ctx, localNamespaceUris[i], false);
        if(res != UA_STATUSCODE_GOOD)
            return false;
    }
    return true;
}

static bool
addAllRefs(void *contextPtr, NL_Node *node) {
    AddNodeContext *context = (AddNodeContext*)contextPtr;
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_StatusCode res =
            UA_Server_addReference(context->server, node->id, ref->refType,
                                   target, ref->isForward);
        if(res != UA_STATUSCODE_GOOD &&
           res != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER,
                           "NodesetLoader: Could not add reference from %N "
                           "to %N (%s)", node->id, ref->target,
                           UA_StatusCode_name(res));
    }
    return true;
}

static bool
addNodes(NodesetLoader *loader, AddNodeContext *anc) {

    // Add all nodes with their type definition and parent
    (void)NodesetLoader_forEachNode(loader, anc, addNodeImpl);

    // Add additional non-hierarchical references
    (void)NodesetLoader_forEachNode(loader, anc, addAllRefs);

    // Call AddNode_finish for all nodes
    (void)NodesetLoader_forEachNode(loader, anc, addNodeFinish);

    return true;
}

bool
NodesetLoader_loadFile(struct UA_Server *server, const char *path,
                       void *options) {
    (void)options;
    if(!server || !path)
        return false;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Logger *logger = config->logging;

    AddNodeContext ctx;
    UA_StatusCode res = AddNodeContext_init(&ctx, server, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Could not initialize import (%s)",
                     UA_StatusCode_name(res));
        AddNodeContext_clear(&ctx);
        return false;
    }
    NodesetLoader *loader = NodesetLoader_new(logger);
    if(!loader) {
        AddNodeContext_clear(&ctx);
        return false;
    }

    NL_FileContext handler;
    memset(&handler, 0, sizeof(NL_FileContext));
    handler.addNamespace = NodesetLoader_BackendOpen62541_addNamespace;
    handler.userContext = &ctx;
    handler.file = path;
    handler.nsMapping = &ctx.nsMapping; // Provide the pre-filled mapping

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SERVER,
                 "NodesetLoader: Start import nodeset: %s", path);
    bool status = NodesetLoader_importFile(loader, &handler);
    if(status)
        status = NodesetLoader_sort(loader);
    if(status)
        status = addNodes(loader, &ctx);
    if(!status)
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Importing the nodeset failed, nodes were not added");
    NodesetLoader_delete(loader);
    AddNodeContext_clear(&ctx);
    return status;
}
