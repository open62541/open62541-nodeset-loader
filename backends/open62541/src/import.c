/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/server.h>

#include <NodesetLoader/backendOpen62541.h>

#include "DataTypeImporter.h"
#include "ServerContext.h"
#include "conversion.h"
#include "NodesetLoader/NodesetLoader.h"
#include "Node.h"

#include <assert.h>

typedef struct {
    ServerContext *serverContext;
    size_t addedCount;
    size_t errorCount;
    UA_NamespaceMapping *nsMapping;
    NodesetLoader_Logger *logger;
    DataTypeImporter *importer;

    // ReferenceTypes that can point to a parent.
    // Inherited from HasChild.
    size_t parentRefTypesSize;
    UA_ExpandedNodeId *parentRefTypes;
} AddNodeContext;

static UA_NodeId
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
                            node->extension, NULL);
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
                                 *qn, attr, node->extension, NULL);
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
                                   node->extension, NULL);
}

static size_t
getArrayDimensions(const char *s, UA_UInt32 **dims) {
    size_t length = strlen(s);
    size_t arrSize = 0;
    if (0 == length)
        return 0;
    // add the first one
    int val = atoi(s);
    arrSize++;
    *dims = (UA_UInt32 *)malloc(sizeof(UA_UInt32));
    (*dims)[0] = (UA_UInt32)val;

    const char *subString = strchr(s, ',');

    while (subString != NULL) {
        arrSize++;
        *dims = (UA_UInt32 *)realloc(*dims, arrSize * sizeof(UA_UInt32));
        (*dims)[arrSize - 1] = (UA_UInt32)atoi(subString + 1);
        subString = strchr(subString + 1, ',');
    }
    return arrSize;
}

static UA_StatusCode
handleVariableNode(const NL_VariableNode *node, UA_NodeId *id,
                   const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt,
                   const UA_QualifiedName *qn,
                   const UA_LocalizedText *description,
                   AddNodeContext *context) {
    UA_Server *server = ServerContext_getServerObject(context->serverContext);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = *lt;
    attr.dataType = node->datatype;
    attr.valueRank = atoi(node->valueRank);
    UA_UInt32 *arrDims = NULL;
    attr.arrayDimensionsSize =
        getArrayDimensions(node->arrayDimensions, &arrDims);
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

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(node->value.length > 0) {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        UA_DecodeXmlOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
        opts.unwrapped = true;
        opts.customTypes = sc->customDataTypes;
        opts.namespaceMapping = context->nsMapping;
        ret = UA_decodeXml(&node->value, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);
        if(ret != UA_STATUSCODE_GOOD) {
            context->logger->log(context->logger->context, NODESETLOADER_LOGLEVEL_WARNING,
                                 "Failed to parse the value of %s", buf);
        }
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
                                  node->extension, NULL);
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
                                       oAttr, node->extension, NULL);
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
                                   *qn, attr, node->extension, NULL);
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
                                  node->extension, NULL);
}

static UA_StatusCode
handleDataTypeNode(const NL_DataTypeNode *node, UA_NodeId *id,
                   const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt,
                   const UA_QualifiedName *qn,
                   const UA_LocalizedText *description,
                   UA_Server *server) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.isAbstract = isValTrue(node->isAbstract);

    return UA_Server_addDataTypeNode(server, *id, *parentId,
                                     *parentReferenceId, *qn,
                                     attr, node->extension, NULL);
}

static void
addNodeImpl(AddNodeContext *context, NL_Node *node) {
    if(node->isDone)
        return;

    context->logger->log(context->logger->context, NODESETLOADER_LOGLEVEL_WARNING, "Adding %N", node->id);

    UA_NodeId id = node->id;
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(context, node, &parentReferenceId);
    UA_LocalizedText lt = node->displayName;
    UA_QualifiedName qn = node->browseName;
    UA_LocalizedText description = node->description;

    UA_Server *server = ServerContext_getServerObject(context->serverContext);

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
        res = handleDataTypeNode((const NL_DataTypeNode *)node, &id, &parentId,
                                 &parentReferenceId, &lt, &qn, &description, server);
        if(res == UA_STATUSCODE_GOOD) {
            const UA_NodeId parent = getParentId(context, node, NULL);
            DataTypeImporter_addCustomDataType(context->importer,
                                               (NL_DataTypeNode *)node, parent);
        }
        break;
    case NODECLASS_VIEW:
        res = handleViewNode((const NL_ViewNode *)node, &id, &parentId,
                             &parentReferenceId, &lt, &qn, &description, server);
        break;
    }

    if (res == UA_STATUSCODE_GOOD) {
        context->addedCount++;
        node->isDone = true;
    } else {
        context->errorCount++;
    }
}

static void
NodesetLoader_BackendOpen62541_addNamespace(void *userContext,
                                            size_t localNamespaceUrisSize,
                                            UA_String *localNamespaceUris,
                                            UA_NamespaceMapping *nsMapping) {
    ServerContext *serverContext = (ServerContext *)userContext;
    UA_Server *server = ServerContext_getServerObject(serverContext);

    /* Build up the mapping tables */
    UA_NamespaceMapping_clear(nsMapping);

    nsMapping->remote2local = (UA_UInt16*)UA_calloc(localNamespaceUrisSize, sizeof(UA_UInt16));
    nsMapping->remote2localSize = localNamespaceUrisSize;

    /* Add all the namespaces */
    char nsbuf[256];
    for(size_t i = 0; i < localNamespaceUrisSize; i++) {
        memcpy(nsbuf, localNamespaceUris[i].data, localNamespaceUris[i].length);
        nsbuf[localNamespaceUris[i].length] = 0;
        nsMapping->remote2local[i] = UA_Server_addNamespace(server, nsbuf);
    }
}

static void
logToOpen(void *context, enum NodesetLoader_LogLevel level,
          const char *message, ...) {
    UA_Logger *logger = (UA_Logger *)context;
    va_list vl;
    va_start(vl, message);
    UA_LogLevel uaLevel = UA_LOGLEVEL_DEBUG;
    switch (level) {
    case NODESETLOADER_LOGLEVEL_DEBUG:
        uaLevel = UA_LOGLEVEL_DEBUG;
        break;
    case NODESETLOADER_LOGLEVEL_ERROR:
        uaLevel = UA_LOGLEVEL_ERROR;
        break;
    case NODESETLOADER_LOGLEVEL_WARNING:
        uaLevel = UA_LOGLEVEL_WARNING;
        break;
    }
    logger->log(logger->context, uaLevel, UA_LOGCATEGORY_USERLAND, message, vl);
    va_end(vl);
}

static void
addAllRefs(UA_Server *server, NL_Node *node) {
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        UA_NodeId src = node->id;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_NodeId refType = ref->refType;
        UA_Server_addReference(server, src, refType, target, ref->isForward);
    }
}

static bool
addNodes(NodesetLoader *loader, NL_FileContext *handler,
         UA_Server *server, ServerContext *serverContext, NodesetLoader_Logger *logger) {
    UA_Boolean done = false;
    AddNodeContext context;
    memset(&context, 0, sizeof(AddNodeContext));
    context.serverContext = serverContext;
    context.nsMapping = &handler->nsMapping;
    context.logger = logger;
    context.importer = DataTypeImporter_new(server);

    // Get all ReferenceTypes that can point to the parent
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeId = UA_NS0ID(HASCHILD);

    UA_Server_browseRecursive(server, &bd,
                              &context.parentRefTypesSize,
                              &context.parentRefTypes);

    // Include HasChild itself
    UA_ExpandedNodeId hasChildExp;
    UA_ExpandedNodeId_init(&hasChildExp);
    hasChildExp.nodeId = UA_NS0ID(HASCHILD);
    UA_Array_append((void**)&context.parentRefTypes,
                    &context.parentRefTypesSize, &hasChildExp,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

    // Ideally we do a linearization (graph sort) before adding the nodes in the
    // right order. As a workaround we mark nodes as "done" and retry until all
    // nodes are added or we cannot add more nodes. This handles cases where a
    // child node appears first in the list. But we get to see some ugly log
    // messages that would not be needed with the graph sort.
 add_nodes:
    context.addedCount = 0;
    context.errorCount = 0;
    NodesetLoader_forEachNode(loader, &context,
                              (NodesetLoader_forEachNode_Func)addNodeImpl);

    // Errors remain.
    // Maybe we needed to add the parents first. Retry.
    if(context.errorCount > 0) {
        // No progress. Bail out.
        /* if(context.addedCount == 0) */
        /*     goto errout; */
        logger->log(logger->context, NODESETLOADER_LOGLEVEL_WARNING,
                    "XXX Some nodes could not be added. "
                    "Try again if the parents now exit.");
        goto errout;
    }

    // Add additional non-hierarchical references
    NodesetLoader_forEachNode(loader, ServerContext_getServerObject(serverContext),
                              (NodesetLoader_forEachNode_Func)addAllRefs);
    done = true;

 errout:
    DataTypeImporter_delete(context.importer);
    return done;
}

bool
NodesetLoader_loadFile(struct UA_Server *server, const char *path,
                       NodesetLoader_ExtensionInterface *extensionHandling) {
    if(!server)
        return false;

    if (!path)
        return false;

    ServerContext *serverContext = ServerContext_new(server);

    NL_FileContext handler;
    memset(&handler, 0, sizeof(NL_FileContext));
    handler.addNamespace = NodesetLoader_BackendOpen62541_addNamespace;
    handler.userContext = serverContext;
    handler.file = path;
    handler.extensionHandling = extensionHandling;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    NodesetLoader_Logger *logger =
        (NodesetLoader_Logger *)calloc(1, sizeof(NodesetLoader_Logger));
#if UA_OPEN62541_VER_MAJOR == 1 && UA_OPEN62541_VER_MINOR < 4
    logger->context = (void*)(uintptr_t)&config->logger;
#else
    logger->context = (void*)(uintptr_t)config->logging;
#endif
    logger->log = &logToOpen;

    NodesetLoader *loader = NodesetLoader_new(logger);
    logger->log(logger->context, NODESETLOADER_LOGLEVEL_DEBUG,
                "Start import nodeset: %s", path);
    bool status = NodesetLoader_importFile(loader, &handler);
    if(status)
        status = NodesetLoader_sort(loader);
    if(status)
        status = addNodes(loader, &handler, server, serverContext, logger);
    if(!status)
        logger->log(logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                    "Importing the nodeset failed, nodes were not added");
    NodesetLoader_delete(loader);
    ServerContext_delete(serverContext);
    UA_NamespaceMapping_clear(&handler.nsMapping);
    free(logger);
    return status;
}
