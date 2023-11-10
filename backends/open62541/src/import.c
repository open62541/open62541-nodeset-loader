/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/server.h>

#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

#include "DataTypeImporter.h"
#include "Value.h"
#include "ServerContext.h"
#include "conversion.h"
#include "NodesetLoader/NodesetLoader.h"
#include "RefServiceImpl.h"

#include <assert.h>

unsigned short NodesetLoader_BackendOpen62541_addNamespace(void *userContext, const char *namespaceUri);

static UA_NodeId getParentDataType(UA_Server *server, const UA_NodeId id)
{
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = id;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.nodeClassMask = UA_NODECLASS_DATATYPE;

    UA_BrowseResult br = UA_Server_browse(server, 10, &bd);
    if (br.statusCode != UA_STATUSCODE_GOOD || br.referencesSize != 1)
    {
        return UA_NODEID_NULL;
    }
    UA_NodeId parentId = br.references[0].nodeId.nodeId;
    UA_BrowseResult_clear(&br);
    return parentId;
}

static bool isKnownParent(const UA_NodeId typeId)
{
    if (typeId.namespaceIndex == 0 &&
        typeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
        typeId.identifier.numeric <= 29)
    {
        return true;
    }
    UA_NodeId optionSetId = UA_NODEID_NUMERIC(0, UA_NS0ID_OPTIONSET);
    if (UA_NodeId_equal(&typeId, &optionSetId))
    {
        return true;
    }
    return false;
}

static UA_NodeId getParentType(UA_Server *server, const UA_NodeId dataTypeId)
{
    UA_NodeId current = dataTypeId;
    while (!isKnownParent(current))
    {
        current = getParentDataType(server, current);
    }
    return current;
}

static UA_NodeId getReferenceTypeId(const NL_Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    return ref->refType;
}

static UA_NodeId getReferenceTarget(const NL_Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    return ref->target;
}

static NL_Reference *getHierachicalInverseReference(const NL_Node *node)
{

    NL_Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef)
    {
        if (!hierachicalRef->isForward)
        {
            return hierachicalRef;
        }
        hierachicalRef = hierachicalRef->next;
    }
    return NULL;
}

static UA_NodeId getParentId(const NL_Node *node, UA_NodeId *parentRefId)
{
    UA_NodeId parentId = UA_NODEID_NULL;

    if(NodesetLoader_isInstanceNode(node))
    {
        parentId = ((const NL_InstanceNode*)node)->parentNodeId;
    }
    NL_Reference *ref = getHierachicalInverseReference((const NL_Node *)node);
    *parentRefId = getReferenceTypeId(ref);
    if (UA_NodeId_equal(&parentId, &UA_NODEID_NULL))
    {
        parentId = getReferenceTarget(ref);
    }
    return parentId;
}

static void
handleObjectNode(const NL_ObjectNode *node, UA_NodeId *id,
                 const UA_NodeId *parentId, const UA_NodeId *parentReferenceId,
                 const UA_LocalizedText *lt, const UA_QualifiedName *qn,
                 const UA_LocalizedText *description, UA_Server *server)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = *lt;
    oAttr.description = *description;
    oAttr.eventNotifier = (UA_Byte)atoi(node->eventNotifier);

    UA_NodeId typeDefId = UA_NODEID_NULL;
    if (node->refToTypeDef)
    {
        typeDefId = node->refToTypeDef->target;
    }

    // addNode_begin is used, otherwise all mandatory childs from type are
    // instantiated
    UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, *id, *parentId,
                            *parentReferenceId, *qn, typeDefId, &oAttr,
                            &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            node->extension, NULL);
}

static void
handleViewNode(const NL_ViewNode *node, UA_NodeId *id, const UA_NodeId *parentId,
               const UA_NodeId *parentReferenceId, const UA_LocalizedText *lt,
               const UA_QualifiedName *qn, const UA_LocalizedText *description,
               UA_Server *server)
{
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.eventNotifier = (UA_Byte)atoi(node->eventNotifier);
    attr.containsNoLoops = isValTrue(node->containsNoLoops);
    UA_Server_addViewNode(server, *id, *parentId, *parentReferenceId, *qn, attr,
                          node->extension, NULL);
}

static void
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

    UA_Server_addMethodNode(server, *id, *parentId, *parentReferenceId, *qn,
                            attr, NULL, 0, NULL, 0, NULL, node->extension,
                            NULL);
}

static size_t getArrayDimensions(const char *s, UA_UInt32 **dims)
{
    size_t length = strlen(s);
    size_t arrSize = 0;
    if (0 == length)
    {
        return 0;
    }
    // add the first one
    int val = atoi(s);
    arrSize++;
    *dims = (UA_UInt32 *)malloc(sizeof(UA_UInt32));
    (*dims)[0] = (UA_UInt32)val;

    const char *subString = strchr(s, ',');

    while (subString != NULL)
    {
        arrSize++;
        *dims = (UA_UInt32 *)realloc(*dims, arrSize * sizeof(UA_UInt32));
        (*dims)[arrSize - 1] = (UA_UInt32)atoi(subString + 1);
        subString = strchr(subString + 1, ',');
    }
    return arrSize;
}

static void handleVariableNode(const NL_VariableNode *node, UA_NodeId *id,
                               const UA_NodeId *parentId,
                               const UA_NodeId *parentReferenceId,
                               const UA_LocalizedText *lt,
                               const UA_QualifiedName *qn,
                               const UA_LocalizedText *description,
                               const ServerContext *serverContext)
{
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

    // this case is only needed for the euromap83 comparison, think the nodeset
    // is not valid
    if (attr.arrayDimensions == NULL && attr.valueRank == 1)
    {
        attr.arrayDimensionsSize = 1;
        attr.arrayDimensions = UA_UInt32_new();
        *attr.arrayDimensions = 0;
    }

    if (attr.arrayDimensionsSize == 0 && node->value && node->value->isArray)
    {
        attr.arrayDimensions = UA_UInt32_new();
        *attr.arrayDimensions =
            (UA_UInt32)node->value->data->val.complexData.membersSize;
        attr.arrayDimensionsSize = 1;
    }
    RawData *data = NULL;
    if (node->value && node->value->data != NULL)
    {
        const UA_DataType *dataType = UA_findDataType(&attr.dataType);
        if (!dataType)
        {
            // try it with custom types
            dataType = NodesetLoader_getCustomDataType(ServerContext_getServerObject(serverContext), &attr.dataType);
            // try it with parent
            if (!dataType)
            {
                const UA_NodeId parent = getParentType(ServerContext_getServerObject(serverContext), attr.dataType);
                dataType = UA_findDataType(&parent);
            }
        }

        UA_ServerConfig *config = UA_Server_getConfig(ServerContext_getServerObject(serverContext));
        const UA_DataTypeArray *types = config->customDataTypes;

        data = RawData_new(data);
        Value_getData(data, node->value, dataType, types->types, serverContext);

        if (data)
        {
            if (node->value->isArray)
            {
                UA_Variant_setArray(
                    &attr.value, data->mem,
                    node->value->data->val.complexData.membersSize, dataType);
            }
            else
            {
                UA_Variant_setScalar(&attr.value, data->mem, dataType);
            }
        }
    }
    UA_NodeId typeDefId = UA_NODEID_NULL;
    if (node->refToTypeDef)
    {
        typeDefId = node->refToTypeDef->target;
    }

    //value is copied by open62541
    UA_Server_addNode_begin(ServerContext_getServerObject(serverContext), UA_NODECLASS_VARIABLE, *id, *parentId,
                            *parentReferenceId, *qn, typeDefId, &attr,
                            &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                            node->extension, NULL);
    //cannot call addNode finish, otherwise the nodes for e.g. range will be instantiated twice
    //UA_Server_addNode_finish(server, *id);
    UA_Variant_clear(&attr.value);
    
    RawData_delete(data);
    UA_free(attr.arrayDimensions);
}

static void handleObjectTypeNode(const NL_ObjectTypeNode *node, UA_NodeId *id,
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

    UA_Server_addObjectTypeNode(server, *id, *parentId, *parentReferenceId, *qn,
                                oAttr, node->extension, NULL);
}

static void handleReferenceTypeNode(const NL_ReferenceTypeNode *node,
                                    UA_NodeId *id, const UA_NodeId *parentId,
                                    const UA_NodeId *parentReferenceId,
                                    const UA_LocalizedText *lt,
                                    const UA_QualifiedName *qn,
                                    const UA_LocalizedText *description,
                                    UA_Server *server)
{
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.symmetric = isValTrue(node->symmetric);
    attr.displayName = *lt;
    attr.description = *description;
    attr.inverseName =
        UA_LOCALIZEDTEXT(node->inverseName.locale, node->inverseName.text);

    UA_Server_addReferenceTypeNode(server, *id, *parentId, *parentReferenceId,
                                   *qn, attr, node->extension, NULL);
}

static void handleVariableTypeNode(const NL_VariableTypeNode *node, UA_NodeId *id,
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
    if (attr.valueRank >= 0)
    {
        if (!strcmp(node->arrayDimensions, ""))
        {
            attr.arrayDimensionsSize = 1;
            UA_UInt32 arrayDimensions[1];
            arrayDimensions[0] = 0;
            attr.arrayDimensions = &arrayDimensions[0];
        }
    }

    UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLETYPE, *id, *parentId,
                            *parentReferenceId, *qn, UA_NODEID_NULL, &attr,
                            &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                            node->extension, NULL);
}

static void handleDataTypeNode(const NL_DataTypeNode *node, UA_NodeId *id,
                               const UA_NodeId *parentId,
                               const UA_NodeId *parentReferenceId,
                               const UA_LocalizedText *lt,
                               const UA_QualifiedName *qn,
                               const UA_LocalizedText *description,
                               UA_Server *server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;
    attr.description = *description;
    attr.isAbstract = isValTrue(node->isAbstract);

    UA_Server_addDataTypeNode(server, *id, *parentId, *parentReferenceId, *qn,
                              attr, node->extension, NULL);
}

static void addNodeImpl(ServerContext *serverContext, const NL_Node *node)
{
    UA_NodeId id = node->id;
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(node, &parentReferenceId);
    UA_LocalizedText lt =
        UA_LOCALIZEDTEXT(node->displayName.locale, node->displayName.text);
    UA_QualifiedName qn =
        UA_QUALIFIEDNAME(node->browseName.nsIdx, node->browseName.name);
    UA_LocalizedText description =
        UA_LOCALIZEDTEXT(node->description.locale, node->description.text);

    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        handleObjectNode((const NL_ObjectNode *)node, &id, &parentId,
                         &parentReferenceId, &lt, &qn, &description, ServerContext_getServerObject(serverContext));
        break;

    case NODECLASS_METHOD:
        handleMethodNode((const NL_MethodNode *)node, &id, &parentId,
                         &parentReferenceId, &lt, &qn, &description, ServerContext_getServerObject(serverContext));
        break;

    case NODECLASS_OBJECTTYPE:
        handleObjectTypeNode((const NL_ObjectTypeNode *)node, &id, &parentId,
                             &parentReferenceId, &lt, &qn, &description,
                             ServerContext_getServerObject(serverContext));
        break;

    case NODECLASS_REFERENCETYPE:
        handleReferenceTypeNode((const NL_ReferenceTypeNode *)node, &id,
                                &parentId, &parentReferenceId, &lt, &qn,
                                &description, ServerContext_getServerObject(serverContext));
        break;

    case NODECLASS_VARIABLETYPE:
        handleVariableTypeNode((const NL_VariableTypeNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, &description,
                               ServerContext_getServerObject(serverContext));
        break;

    case NODECLASS_VARIABLE:
        handleVariableNode((const NL_VariableNode *)node, &id, &parentId,
                           &parentReferenceId, &lt, &qn, &description, serverContext);
        break;
    case NODECLASS_DATATYPE:
        handleDataTypeNode((const NL_DataTypeNode *)node, &id, &parentId,
                           &parentReferenceId, &lt, &qn, &description, ServerContext_getServerObject(serverContext));
        break;
    case NODECLASS_VIEW:
        handleViewNode((const NL_ViewNode *)node, &id, &parentId,
                       &parentReferenceId, &lt, &qn, &description, ServerContext_getServerObject(serverContext));
        break;
    }
}

unsigned short
NodesetLoader_BackendOpen62541_addNamespace(void *userContext, const char *namespaceUri) {
    ServerContext *serverContext = (ServerContext *)userContext;

    UA_UInt16 idx =
        UA_Server_addNamespace(ServerContext_getServerObject(serverContext), namespaceUri);

    ServerContext_addNamespaceIdx(serverContext, idx);

    return idx;
}

static void logToOpen(void *context, enum NodesetLoader_LogLevel level,
                      const char *message, ...)
{
    UA_Logger *logger = (UA_Logger *)context;
    va_list vl;
    va_start(vl, message);
    UA_LogLevel uaLevel = UA_LOGLEVEL_DEBUG;
    switch (level)
    {
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

struct DataTypeImportCtx
{
    DataTypeImporter *importer;
    const NL_BiDirectionalReference *hasEncodingRef;
    UA_Server *server;
};

static void addDataType(struct DataTypeImportCtx *ctx, NL_Node *node)
{
    // add only the types
    const NL_BiDirectionalReference *r = ctx->hasEncodingRef;
    while (r)
    {
        if (UA_NodeId_equal(&r->source, &node->id))
        {
            NL_Reference *ref = (NL_Reference *)calloc(1, sizeof(NL_Reference));
            ref->refType = r->refType;
            ref->target = r->target;

            NL_Reference *lastRef = node->nonHierachicalRefs;
            node->nonHierachicalRefs = ref;
            ref->next = lastRef;
            break;
        }
        r = r->next;
    }
    const UA_NodeId parent =
        getParentType(ctx->server, node->id);
    DataTypeImporter_addCustomDataType(ctx->importer, (NL_DataTypeNode *)node,
                                       parent);
}

static void importDataTypes(NodesetLoader *loader, UA_Server *server)
{
    // add datatypes
    const NL_BiDirectionalReference *hasEncodingRef =
        NodesetLoader_getBidirectionalRefs(loader);
    DataTypeImporter *importer = DataTypeImporter_new(server);
    struct DataTypeImportCtx ctx;
    ctx.hasEncodingRef = hasEncodingRef;
    ctx.server = server;
    ctx.importer = importer;
    NodesetLoader_forEachNode(loader, NODECLASS_DATATYPE, &ctx,
                              (NodesetLoader_forEachNode_Func)addDataType);

    DataTypeImporter_initMembers(importer);
    DataTypeImporter_delete(importer);
}

static void addNonHierachicalRefs(UA_Server *server, NL_Node *node)
{
    NL_Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {

        UA_NodeId src = node->id;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_NodeId refType = ref->refType;
        UA_Server_addReference(server, src, refType, target, ref->isForward);
        ref = ref->next;
    }
    // brute force, maybe not the best way to do this
    ref = node->hierachicalRefs;
    while (ref)
    {
        UA_NodeId src = node->id;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_NodeId refType = ref->refType;
        UA_Server_addReference(server, src, refType, target, ref->isForward);
        ref = ref->next;
    }
}

static void addNodes(NodesetLoader *loader, ServerContext *serverContext,
                     NodesetLoader_Logger *logger)
{
    const NL_NodeClass order[NL_NODECLASS_COUNT] = {
        NODECLASS_REFERENCETYPE, NODECLASS_DATATYPE, NODECLASS_OBJECTTYPE,
        NODECLASS_VARIABLETYPE,  NODECLASS_OBJECT,   NODECLASS_METHOD,
        NODECLASS_VARIABLE,      NODECLASS_VIEW};

    for (size_t i = 0; i < NL_NODECLASS_COUNT; i++)
    {
        const NL_NodeClass classToImport = order[i];
        size_t cnt =
            NodesetLoader_forEachNode(loader, classToImport, serverContext,
                                      (NodesetLoader_forEachNode_Func)addNodeImpl);
        if (classToImport == NODECLASS_DATATYPE)
        {
            importDataTypes(loader, ServerContext_getServerObject(serverContext));
        }
        logger->log(logger->context, NODESETLOADER_LOGLEVEL_DEBUG,
                    "imported %ss: %zu", NL_NODECLASS_NAME[classToImport], cnt);
    }

    for (size_t i = 0; i < NL_NODECLASS_COUNT; i++)
    {
        const NL_NodeClass classToImport = order[i];
        NodesetLoader_forEachNode(
            loader, classToImport, ServerContext_getServerObject(serverContext),
            (NodesetLoader_forEachNode_Func)addNonHierachicalRefs);
    }
}

bool NodesetLoader_loadFile(struct UA_Server *server, const char *path,
                            NodesetLoader_ExtensionInterface *extensionHandling)
{
    if (!server)
    {
        return false;
    }
    if (!path)
    {
        return false;
    }

    ServerContext *serverContext = ServerContext_new(server);

    NL_FileContext handler;
    handler.addNamespace = NodesetLoader_BackendOpen62541_addNamespace;
    handler.userContext = serverContext;
    handler.file = path;
    handler.extensionHandling = extensionHandling;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    NodesetLoader_Logger *logger =
        (NodesetLoader_Logger *)calloc(1, sizeof(NodesetLoader_Logger));
    logger->context = config->logging;
    logger->log = &logToOpen;
    NL_ReferenceService *refService = RefServiceImpl_new(server);

    NodesetLoader *loader = NodesetLoader_new(logger, refService);
    logger->log(logger->context, NODESETLOADER_LOGLEVEL_DEBUG,
                "Start import nodeset: %s", path);
    bool importStatus = NodesetLoader_importFile(loader, &handler);
    bool sortStatus = NodesetLoader_sort(loader);
    bool retStatus = importStatus && sortStatus;
    if (retStatus && sortStatus)
    {
        addNodes(loader, serverContext, logger);
    }
    else
    {
        logger->log(logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                    "importing the nodeset failed, nodes were not added");
    }
    RefServiceImpl_delete(refService);
    NodesetLoader_delete(loader);
    ServerContext_delete(serverContext);
    free(logger);
    return retStatus;
}
