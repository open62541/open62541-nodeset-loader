/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/server.h>

#include <NodesetLoader/backendOpen62541.h>

#include "ServerContext.h"
#include "NodesetLoader/NodesetLoader.h"
#include "RefServiceImpl.h"
#include "Node.h"

#include <assert.h>

static inline UA_Boolean isValTrue(const char *s) {
    if(!s)
        return UA_FALSE;
    if(strcmp(s, "true"))
        return false;
    return true;
}

static UA_NodeId getReferenceTypeId(const NL_Reference *ref) {
    if (!ref)
        return UA_NODEID_NULL;
    return ref->refType;
}

static UA_NodeId getReferenceTarget(const NL_Reference *ref) {
    if (!ref)
        return UA_NODEID_NULL;
    return ref->target;
}

static NL_Reference *getHierachicalInverseReference(const NL_Node *node)
{

    NL_Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef) {
        if (!hierachicalRef->isForward)
            return hierachicalRef;
        hierachicalRef = hierachicalRef->next;
    }
    return NULL;
}

static UA_NodeId getParentId(const NL_Node *node, UA_NodeId *parentRefId) {
    UA_NodeId parentId = UA_NODEID_NULL;

    if(NodesetLoader_isInstanceNode(node))
        parentId = ((const NL_InstanceNode*)node)->parentNodeId;
    NL_Reference *ref = getHierachicalInverseReference((const NL_Node *)node);
    *parentRefId = getReferenceTypeId(ref);
    if (UA_NodeId_equal(&parentId, &UA_NODEID_NULL))
        parentId = getReferenceTarget(ref);
    return parentId;
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

    UA_NodeId typeDefId = UA_NODEID_NULL;
    if (node->refToTypeDef)
        typeDefId = node->refToTypeDef->target;

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
    return UA_Server_addViewNode(server, *id, *parentId, *parentReferenceId, *qn, attr,
                          node->extension, NULL);
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
    return UA_Server_addMethodNode(server, *id, *parentId, *parentReferenceId, *qn,
                            attr, NULL, 0, NULL, 0, NULL, node->extension, NULL);
}

static size_t getArrayDimensions(const char *s, UA_UInt32 **dims) {
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
    UA_Server *server = context->server;

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

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_DecodeXmlOptions opts;
    memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
    opts.unwrapped = true;
    opts.customTypes = sc->customDataTypes;
    opts.namespaceMapping = &context->nsMapping;
    UA_StatusCode ret =
        UA_decodeXml(&node->value, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);
    if(ret != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(context->logger, UA_LOGCATEGORY_SERVER, "NodesetLoader: Failed to parse the value of %s", buf);

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

    UA_NodeId typeDefId = UA_NODEID_NULL;
    if(node->refToTypeDef)
        typeDefId = node->refToTypeDef->target;

    //value is copied by open62541
    UA_StatusCode res =
        UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, *id, *parentId,
                                *parentReferenceId, *qn, typeDefId, &attr,
                                &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                                node->extension, NULL);
    //cannot call addNode finish, otherwise the nodes for e.g. range will be instantiated twice
    //UA_Server_addNode_finish(server, *id);

    UA_Variant_clear(&attr.value);
    if(attr.arrayDimensions && attr.arrayDimensions != &arrayDims)
        UA_free(attr.arrayDimensions);
    return res;
}

static UA_StatusCode handleObjectTypeNode(const NL_ObjectTypeNode *node, UA_NodeId *id,
                                 const UA_NodeId *parentId,
                                 const UA_NodeId *parentReferenceId,
                                 const UA_LocalizedText *lt,
                                 const UA_QualifiedName *qn,
                                 const UA_LocalizedText *description,
                                 UA_Server *server)
{
    UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
    oAttr.displayName = *lt;
    oAttr.isAbstract = isValTrue(node->isAbstract);
    oAttr.description = *description;

    return UA_Server_addObjectTypeNode(server, *id, *parentId, *parentReferenceId, *qn,
                                oAttr, node->extension, NULL);
}

static UA_StatusCode handleReferenceTypeNode(const NL_ReferenceTypeNode *node,
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

static UA_StatusCode handleVariableTypeNode(const NL_VariableTypeNode *node, UA_NodeId *id,
                                   const UA_NodeId *parentId,
                                   const UA_NodeId *parentReferenceId,
                                   const UA_LocalizedText *lt,
                                   const UA_QualifiedName *qn,
                                   const UA_LocalizedText *description,
                                   UA_Server *server)
{
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

   return UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLETYPE, *id, *parentId,
                            *parentReferenceId, *qn, UA_NODEID_NULL, &attr,
                            &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                            node->extension, NULL);
}

static UA_StatusCode
handleDataTypeNode(AddNodeContext *ctx,
                   const NL_DataTypeNode *node, UA_NodeId *id,
                   const UA_NodeId *parentId,
                   const UA_NodeId *parentReferenceId,
                   const UA_LocalizedText *lt,
                   const UA_QualifiedName *qn,
                   const UA_LocalizedText *description) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->server);

    return UA_STATUSCODE_GOOD;

    UA_StructureDefinition *sd = NULL;

    // Generate the UA_DataType
    UA_DataType type;
    memset(&type, 0, sizeof(UA_DataType));
    UA_StatusCode res =
        UA_DataType_fromStructureDefinition(&type, sd, *id, lt->text, sc->customDataTypes);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    // Add the UA_DataType the the server
    res = AddNodeContext_addDataType(ctx, &type);
    if(res != UA_STATUSCODE_GOOD) {
        UA_DataType_clear(&type);
        return res;
    }

    // Add the DataTypeNode
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.isAbstract = isValTrue(node->isAbstract);
    return UA_Server_addDataTypeNode(ctx->server, *id, *parentId,
                                     *parentReferenceId, *qn,
                                     attr, node->extension, NULL);
}

static void
addNodeImpl(AddNodeContext *context, NL_Node *node) {
    UA_NodeId id = node->id;
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(node, &parentReferenceId);
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

    // If a node was not added to the server due to an error, we add such a node
    // to a special node container. We can then try to add such nodes later.
    if(UA_StatusCode_isBad(res))
        NodeContainer_add(&context->problemNodes, node);
}

static void
NodesetLoader_BackendOpen62541_addNamespace(void *userContext,
                                            size_t localNamespaceUrisSize,
                                            UA_String *localNamespaceUris,
                                            UA_NamespaceMapping *nsMapping) {
    AddNodeContext *ctx = (AddNodeContext*)userContext;
    for(size_t i = 0; i < localNamespaceUrisSize; i++) {
        AddNodeContext_addNamespace(ctx, localNamespaceUris[i]);
    }
    *nsMapping = ctx->nsMapping;
}

static void
addNonHierachicalRefs(AddNodeContext *ctx, NL_Node *node) {
    NL_Reference *ref = node->nonHierachicalRefs;
    while(ref) {
        UA_NodeId src = node->id;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_NodeId refType = ref->refType;
        UA_Server_addReference(ctx->server, src, refType, target, ref->isForward);
        ref = ref->next;
    }

    // brute force, maybe not the best way to do this
    ref = node->hierachicalRefs;
    while(ref) {
        UA_NodeId src = node->id;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_NodeId refType = ref->refType;
        UA_Server_addReference(ctx->server, src, refType, target, ref->isForward);
        ref = ref->next;
    }
}

bool NodesetLoader_loadFile(struct UA_Server *server, const char *path,
                            void *options) {
    if(!server || !path)
        return false;

    AddNodeContext *ctx = AddNodeContext_new(server);
    if(!ctx)
        return false;

    NL_FileContext handler;
    memset(&handler, 0, sizeof(NL_FileContext));
    handler.addNamespace = NodesetLoader_BackendOpen62541_addNamespace;
    handler.userContext = ctx;
    handler.file = path;
    handler.nsMapping = ctx->nsMapping; // Provide the pre-filled mapping

    UA_ServerConfig *config = UA_Server_getConfig(server);
    NL_ReferenceService *refService = RefServiceImpl_new(server);

    NodesetLoader *loader = NodesetLoader_new(config->logging, refService);
    UA_LOG_DEBUG(config->logging, UA_LOGCATEGORY_SERVER, "NodesetLoader: "
                "Start import nodeset: %s", path);

    // Import from XML
    bool status = NodesetLoader_importFile(loader, &handler);

    // Sort
    if(status)
        status = NodesetLoader_sort(loader);

    // Load
    if(status) {
        static const NL_NodeClass order[NL_NODECLASS_COUNT] = {
            NODECLASS_REFERENCETYPE, NODECLASS_DATATYPE, NODECLASS_OBJECTTYPE,
            NODECLASS_VARIABLETYPE,  NODECLASS_OBJECT,   NODECLASS_METHOD,
            NODECLASS_VARIABLE,      NODECLASS_VIEW};

        for(size_t i = 0; i < NL_NODECLASS_COUNT; i++) {
            const NL_NodeClass classToImport = order[i];
            NodesetLoader_forEachNode(loader, classToImport, ctx,
                                      (NodesetLoader_forEachNode_Func)addNodeImpl);
        }

        for(size_t i = 0; i < NL_NODECLASS_COUNT; i++) {
            const NL_NodeClass classToImport = order[i];
            NodesetLoader_forEachNode(loader, classToImport, ctx,
                                      (NodesetLoader_forEachNode_Func)addNonHierachicalRefs);
        }
    } else {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER, "NodesetLoader: "
                     "importing the nodeset failed, nodes were not added");
    }

    // Clean up
    RefServiceImpl_delete(refService);
    NodesetLoader_delete(loader);
    AddNodeContext_delete(ctx);

    return UA_STATUSCODE_GOOD;
}
