#include "DataTypeImporter.h"
#include "conversion.h"
#include "value.h"
#include <NodesetLoader/backendOpen62541.h>
#include <dataTypes.h>
#include <NodesetLoader/NodesetLoader.h>
#include <open62541/server.h>
#include <open62541/server_config.h>

int BackendOpen62541_addNamespace(void *userContext, const char *namespaceUri);

static UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node)
{
    Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {
        if (!strcmp("i=40", ref->refType.id))
        {
            return getNodeIdFromChars(ref->target);
        }
        ref = ref->next;
    }
    return UA_NODEID_NULL;
}

static UA_NodeId getReferenceTypeId(const Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    return getNodeIdFromChars(ref->refType);
}

static UA_NodeId getReferenceTarget(const Reference *ref)
{
    if (!ref)
    {
        return UA_NODEID_NULL;
    }
    return getNodeIdFromChars(ref->target);
}

static Reference *getHierachicalInverseReference(const TNode *node)
{

    Reference *hierachicalRef = node->hierachicalRefs;
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

static UA_NodeId getParentId(const TNode *node, UA_NodeId *parentRefId)
{
    UA_NodeId parentId = UA_NODEID_NULL;
    if (node->nodeClass == NODECLASS_OBJECT)
    {
        parentId =
            getNodeIdFromChars(((const TObjectNode *)node)->parentNodeId);
    }
    else if (node->nodeClass == NODECLASS_VARIABLE)
    {
        parentId =
            getNodeIdFromChars(((const TVariableNode *)node)->parentNodeId);
    }
    Reference *ref = getHierachicalInverseReference((const TNode *)node);
    *parentRefId = getReferenceTypeId(ref);
    if (UA_NodeId_equal(&parentId, &UA_NODEID_NULL))
    {
        parentId = getReferenceTarget(ref);
    }
    return parentId;
}

static void handleObjectNode(const TObjectNode *node, UA_NodeId *id,
                             const UA_NodeId *parentId,
                             const UA_NodeId *parentReferenceId,
                             const UA_LocalizedText *lt,
                             const UA_QualifiedName *qn, UA_Server *server)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = *lt;

    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);

    UA_Server_addObjectNode(server, *id, *parentId, *parentReferenceId, *qn,
                            typeDefId, oAttr, NULL, NULL);
}

static void handleMethodNode(const TMethodNode *node, UA_NodeId *id,
                             const UA_NodeId *parentId,
                             const UA_NodeId *parentReferenceId,
                             const UA_LocalizedText *lt,
                             const UA_QualifiedName *qn, UA_Server *server)
{
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = isTrue(node->executable);
    attr.userExecutable = isTrue(node->userExecutable);
    attr.displayName = *lt;

    UA_Server_addMethodNode(server, *id, *parentId, *parentReferenceId, *qn,
                            attr, NULL, 0, NULL, 0, NULL, NULL, NULL);
    Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {
        UA_NodeId refId = getReferenceTypeId(ref);
        UA_ExpandedNodeId eid;
        eid.nodeId = getReferenceTarget(ref);
        eid.namespaceUri = UA_STRING_NULL;
        eid.serverIndex = 0;
        UA_Server_addReference(server, *id, refId, eid, ref->isForward);
        ref = ref->next;
    }
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
    *dims[0] = (UA_UInt32)val;

    const char *subString = strchr(s, ';');

    while (subString != NULL)
    {
        arrSize++;
        *dims = (UA_UInt32 *)realloc(*dims, arrSize * sizeof(UA_UInt32));
        *dims[arrSize - 1] = (UA_UInt32)atoi(subString + 1);
        subString = strchr(subString + 1, ';');
    }
    return arrSize;
}

static void handleVariableNode(const TVariableNode *node, UA_NodeId *id,
                               const UA_NodeId *parentId,
                               const UA_NodeId *parentReferenceId,
                               const UA_LocalizedText *lt,
                               const UA_QualifiedName *qn, UA_Server *server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = *lt;
    attr.dataType = getNodeIdFromChars(node->datatype);
    attr.valueRank = atoi(node->valueRank);
    UA_UInt32 *arrDims = NULL;
    attr.arrayDimensionsSize =
        getArrayDimensions(node->arrayDimensions, &arrDims);
    attr.arrayDimensions = arrDims;
    attr.accessLevel = (UA_Byte)atoi(node->accessLevel);
    attr.userAccessLevel = (UA_Byte)atoi(node->userAccessLevel);

    //euromap work around?
    if(attr.arrayDimensions == NULL && attr.valueRank == 1)
    {
        attr.arrayDimensionsSize = 1;
        attr.arrayDimensions = UA_UInt32_new();
        *attr.arrayDimensions = 0;
    }

    // todo: is this really necessary??
    UA_UInt32 dims = 0;
    if (attr.arrayDimensionsSize == 0 && node->value && node->value->isArray)
    {
        dims = (UA_UInt32)node->value->arrayCnt;
        attr.arrayDimensions = &dims;
        attr.arrayDimensionsSize = 1;
    }
    if (node->value)
    {
        if (node->value->isArray)
        {
            UA_Variant_setArray(&attr.value, node->value->value,
                                node->value->arrayCnt, node->value->datatype);
        }
        else
        {
            UA_Variant_setScalar(&attr.value, node->value->value,
                                 node->value->datatype);
        }
    }
    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);
    UA_Server_addVariableNode(server, *id, *parentId, *parentReferenceId, *qn,
                              typeDefId, attr, NULL, NULL);
    UA_free(attr.arrayDimensions);

    // value is copied in addVariableNode
    BackendOpen62541_Value_delete(&((TVariableNode *)(uintptr_t)node)->value);
}

static void handleObjectTypeNode(const TObjectTypeNode *node, UA_NodeId *id,
                                 const UA_NodeId *parentId,
                                 const UA_NodeId *parentReferenceId,
                                 const UA_LocalizedText *lt,
                                 const UA_QualifiedName *qn, UA_Server *server)
{
    UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
    oAttr.displayName = *lt;
    oAttr.isAbstract = isTrue(node->isAbstract);

    UA_Server_addObjectTypeNode(server, *id, *parentId, *parentReferenceId, *qn,
                                oAttr, NULL, NULL);
}

static void handleReferenceTypeNode(const TReferenceTypeNode *node,
                                    UA_NodeId *id, const UA_NodeId *parentId,
                                    const UA_NodeId *parentReferenceId,
                                    const UA_LocalizedText *lt,
                                    const UA_QualifiedName *qn,
                                    UA_Server *server)
{
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.symmetric = true;
    attr.displayName = *lt;

    UA_Server_addReferenceTypeNode(server, *id, *parentId, *parentReferenceId,
                                   *qn, attr, NULL, NULL);
}

static void handleVariableTypeNode(const TVariableTypeNode *node, UA_NodeId *id,
                                   const UA_NodeId *parentId,
                                   const UA_NodeId *parentReferenceId,
                                   const UA_LocalizedText *lt,
                                   const UA_QualifiedName *qn,
                                   UA_Server *server)
{
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = *lt;
    attr.valueRank = atoi(node->valueRank);
    attr.isAbstract = isTrue(node->isAbstract);
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

    UA_NodeId typeDefId = getTypeDefinitionIdFromChars2((const TNode *)node);

    UA_Server_addVariableTypeNode(server, *id, *parentId, *parentReferenceId,
                                  *qn, typeDefId, attr, NULL, NULL);
}

static void handleDataTypeNode(const TDataTypeNode *node, UA_NodeId *id,
                               const UA_NodeId *parentId,
                               const UA_NodeId *parentReferenceId,
                               const UA_LocalizedText *lt,
                               const UA_QualifiedName *qn, UA_Server *server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = *lt;

    UA_Server_addDataTypeNode(server, *id, *parentId, *parentReferenceId, *qn,
                              attr, NULL, NULL);
}

static void addNode(UA_Server *server, const TNode *node)
{
    UA_NodeId id = getNodeIdFromChars(node->id);
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(node, &parentReferenceId);
    UA_LocalizedText lt = UA_LOCALIZEDTEXT((char *)"", node->displayName);
    UA_QualifiedName qn =
        UA_QUALIFIEDNAME(node->browseName.nsIdx, node->browseName.name);

    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        handleObjectNode((const TObjectNode *)node, &id, &parentId,
                         &parentReferenceId, &lt, &qn, server);
        break;

    case NODECLASS_METHOD:
        handleMethodNode((const TMethodNode *)node, &id, &parentId,
                         &parentReferenceId, &lt, &qn, server);
        break;

    case NODECLASS_OBJECTTYPE:
        handleObjectTypeNode((const TObjectTypeNode *)node, &id, &parentId,
                             &parentReferenceId, &lt, &qn, server);
        break;

    case NODECLASS_REFERENCETYPE:
        handleReferenceTypeNode((const TReferenceTypeNode *)node, &id,
                                &parentId, &parentReferenceId, &lt, &qn,
                                server);
        break;

    case NODECLASS_VARIABLETYPE:
        handleVariableTypeNode((const TVariableTypeNode *)node, &id, &parentId,
                               &parentReferenceId, &lt, &qn, server);
        break;

    case NODECLASS_VARIABLE:
        handleVariableNode((const TVariableNode *)node, &id, &parentId,
                           &parentReferenceId, &lt, &qn, server);
        break;
    case NODECLASS_DATATYPE:
        handleDataTypeNode((const TDataTypeNode *)node, &id, &parentId,
                           &parentReferenceId, &lt, &qn, server);
    }
}

int BackendOpen62541_addNamespace(void *userContext, const char *namespaceUri)
{
    int idx =
        (int)UA_Server_addNamespace((UA_Server *)userContext, namespaceUri);
    return idx;
}

bool NodesetLoader_loadFile(struct UA_Server *server, const char *path,
                            void *extensionHandling)
{
    if (!server)
    {
        return false;
    }
    if (!path)
    {
        return false;
    }
    FileContext handler;
    handler.addNamespace = BackendOpen62541_addNamespace;
    handler.userContext = server;
    handler.file = path;
    ValueInterface valIf;
    valIf.userContext = NULL;
    valIf.newValue = BackendOpen62541_Value_new;
    valIf.start = BackendOpen62541_Value_start;
    valIf.end = BackendOpen62541_Value_end;
    valIf.finish = BackendOpen62541_Value_finish;
    valIf.deleteValue = BackendOpen62541_Value_delete;
    handler.valueHandling = &valIf;
    handler.extensionHandling = NULL;

    NodesetLoader *loader = NodesetLoader_new();
    bool status = NodesetLoader_importFile(loader, &handler);
    NodesetLoader_sort(loader);
    if (status)
    {
        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_REFERENCETYPE, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_DATATYPE, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_OBJECTTYPE, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_OBJECT, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_METHOD, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_VARIABLETYPE, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }

        {
            TNode **nodes = NULL;
            size_t cnt =
                NodesetLoader_getNodes(loader, NODECLASS_VARIABLE, &nodes);
            for (TNode **node = nodes; node != nodes + cnt; node++)
            {
                addNode(server, *node);
            }
        }
    }

    DataTypeImporter *importer = DataTypeImporter_new(server);
    TNode **nodes = NULL;
    size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_DATATYPE, &nodes);
    for (TNode **node = nodes; node != nodes + cnt; node++)
    {
        // add only the types
        const BiDirectionalReference* hasEncodingRef = NodesetLoader_getBidirectionalRefs(loader);
        /*while (hasEncodingRef)
        {
            printf("source %s target %s\n", hasEncodingRef->source.id,
                   hasEncodingRef->target.id);
            hasEncodingRef = hasEncodingRef->next;
        }*/
            while (hasEncodingRef)
            {
                if (!TNodeId_cmp(&hasEncodingRef->source, &(*node)->id))
                {
                    Reference *ref = (Reference *)calloc(1, sizeof(Reference));
                    ref->refType = hasEncodingRef->refType;
                    ref->target = hasEncodingRef->target;

                    Reference *lastRef = (*node)->nonHierachicalRefs;
                    (*node)->nonHierachicalRefs = ref;
                    ref->next = lastRef;
                    break;
                }
                hasEncodingRef = hasEncodingRef->next;
            }
            DataTypeImporter_addCustomDataType(importer,
                                               (TDataTypeNode *)*node);
        }
    DataTypeImporter_initMembers(importer);
    DataTypeImporter_delete(importer);

    NodesetLoader_delete(loader);
    return status;
}

const struct UA_DataType *getCustomDataType(struct UA_Server *server,
                                            const UA_NodeId *typeId)
{
    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_DataTypeArray *types = config->customDataTypes;
    while (types)
    {
        const UA_DataTypeArray *next = types->next;
        if (types->types)
        {
            for (const UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                if (UA_NodeId_equal(&type->typeId, typeId))
                {
                    return type;
                }
            }
        }

        types = next;
    }
    return NULL;
}
