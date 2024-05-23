/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "Nodeset.h"
#include "AliasList.h"
#include "NamespaceList.h"
#include "Sort.h"
#include "nodes/DataTypeNode.h"
#include "nodes/Node.h"
#include "nodes/NodeContainer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UA_NodeId extractNodedId(const NamespaceList *namespaces, char *s);
static UA_NodeId alias2Id(const Nodeset *nodeset, char *name);
static UA_NodeId translateNodeId(const NamespaceList *namespaces, UA_NodeId id);
static NL_BrowseName translateBrowseName(const NamespaceList *namespaces,
                                         NL_BrowseName id);
NL_BrowseName extractBrowseName(const NamespaceList *namespaces, char *s);

// UANode
#define ATTRIBUTE_NODEID "NodeId"
#define ATTRIBUTE_BROWSENAME "BrowseName"
// UAInstance
#define ATTRIBUTE_PARENTNODEID "ParentNodeId"
// UAVariable
#define ATTRIBUTE_DATATYPE "DataType"
#define ATTRIBUTE_VALUERANK "ValueRank"
#define ATTRIBUTE_ARRAYDIMENSIONS "ArrayDimensions"
#define ATTRIBUTE_HISTORIZING "Historizing"
#define ATTRIBUTE_MINIMUMSAMPLINGINTERVAL "MinimumSamplingInterval"
// UAObject
#define ATTRIBUTE_EVENTNOTIFIER "EventNotifier"
// UAObjectType
#define ATTRIBUTE_ISABSTRACT "IsAbstract"
// NL_Reference
#define ATTRIBUTE_REFERENCETYPE "ReferenceType"
#define ATTRIBUTE_ISFORWARD "IsForward"
#define ATTRIBUTE_SYMMETRIC "Symmetric"
#define ATTRIBUTE_ALIAS "Alias"
// View
#define ATTRIBUTE_CONTAINSNOLOOPS "ContainsNoLoops"

typedef struct
{
    const char *name;
    char *defaultValue;
} NodeAttribute;

const NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL};
const NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL};
const NodeAttribute attrParentNodeId = {ATTRIBUTE_PARENTNODEID, NULL};
const NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, "0"};
const NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24"};
const NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1"};
const NodeAttribute attrMinimumSamplingInterval = {ATTRIBUTE_MINIMUMSAMPLINGINTERVAL, "-1"};
const NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, ""};
const NodeAttribute attrIsAbstract = {ATTRIBUTE_ISABSTRACT, "false"};
const NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true"};
const NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL};
const NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL};
const NodeAttribute attrExecutable = {"Executable", "true"};
const NodeAttribute attrUserExecutable = {"UserExecutable", "true"};
const NodeAttribute attrAccessLevel = {"AccessLevel", "1"};
const NodeAttribute attrUserAccessLevel = {"UserAccessLevel", "1"};
const NodeAttribute attrSymmetric = {"Symmetric", "false"};
const NodeAttribute dataTypeDefinition_IsUnion = {"IsUnion", "false"};
const NodeAttribute dataTypeDefinition_IsOptionSet = {"IsOptionSet", "false"};
const NodeAttribute dataTypeField_Name = {"Name", NULL};
const NodeAttribute dataTypeField_DataType = {"DataType", "i=24"};
const NodeAttribute dataTypeField_Value = {"Value", NULL};
const NodeAttribute dataTypeField_IsOptional = {"IsOptional", "false"};
const NodeAttribute attrLocale = {"Locale", NULL};
const NodeAttribute attrHistorizing = {ATTRIBUTE_HISTORIZING, "false"};
const NodeAttribute attrContainsNoLoops = {ATTRIBUTE_CONTAINSNOLOOPS, "false"};

UA_NodeId translateNodeId(const NamespaceList *namespaces, UA_NodeId id)
{
    if (id.namespaceIndex == 0)
    {
        return id;
    }
    const Namespace *ns =
        NamespaceList_getNamespace(namespaces, id.namespaceIndex);
    if (ns)
    {
        id.namespaceIndex = ns->idx;
    }
    return id;
}

NL_BrowseName translateBrowseName(const NamespaceList *namespaces,
                                  NL_BrowseName bn)
{
    if (bn.nsIdx > 0)
    {
        const Namespace *ns = NamespaceList_getNamespace(namespaces, bn.nsIdx);
        if (ns != NULL)
        {
            bn.nsIdx =
                (uint16_t)NamespaceList_getNamespace(namespaces, bn.nsIdx)->idx;
        }
        return bn;
    }
    return bn;
}

UA_NodeId extractNodedId(const NamespaceList *namespaces, char *s)
{
    UA_NodeId id = UA_NODEID_NULL;
    if (s == NULL)
        return id;

    UA_StatusCode res = UA_NodeId_parse(&id, UA_STRING(s));
    if (res != UA_STATUSCODE_GOOD)
        return id;

    return translateNodeId(namespaces, id);
}

NL_BrowseName extractBrowseName(const NamespaceList *namespaces, char *s)
{
    NL_BrowseName bn;
    bn.nsIdx = 0;
    bn.name = NULL;
    if (!s) return bn;
    char *bnName = strchr(s, ':');
    if (bnName == NULL)
    {
        bn.name = s;
        return bn;
    }
    else
    {
        bn.nsIdx = (uint16_t)atoi(&s[0]);
        bn.name = bnName + 1;
    }
    return translateBrowseName(namespaces, bn);
}

static UA_NodeId alias2Id(const Nodeset *nodeset, char *name)
{
    const UA_NodeId *alias = AliasList_getNodeId(nodeset->aliasList, name);
    if (!alias)
    {
        return extractNodedId(nodeset->namespaces, name);
    }
    return *alias;
}

Nodeset *Nodeset_new(NL_addNamespaceCallback nsCallback,
                     NodesetLoader_Logger *logger,
                     NL_ReferenceService *refService)
{
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));
    if (!nodeset)
    {
        return NULL;
    }
    nodeset->aliasList = AliasList_new();
    nodeset->namespaces = NamespaceList_new(nsCallback);
    nodeset->charArena = CharArenaAllocator_new(1024 * 1024);
    nodeset->nodes[NODECLASS_OBJECT] = NodeContainer_new(10000, true);
    nodeset->nodes[NODECLASS_VARIABLE] = NodeContainer_new(10000, true);
    nodeset->nodes[NODECLASS_METHOD] = NodeContainer_new(1000, true);
    nodeset->nodes[NODECLASS_OBJECTTYPE] = NodeContainer_new(100, true);
    nodeset->nodes[NODECLASS_DATATYPE] = NodeContainer_new(100, true);
    nodeset->nodes[NODECLASS_REFERENCETYPE] = NodeContainer_new(100, true);
    nodeset->nodes[NODECLASS_VARIABLETYPE] = NodeContainer_new(100, true);
    nodeset->nodes[NODECLASS_VIEW] = NodeContainer_new(10, true);
    nodeset->nodesWithUnknownRefs = NodeContainer_new(100, false);
    nodeset->refTypesWithUnknownRefs = NodeContainer_new(100, false);
    nodeset->refService = refService;
    nodeset->sortCtx = Sort_init();
    nodeset->logger = logger;
    return nodeset;
}

static void Nodeset_addNode(Nodeset *nodeset, NL_Node *node)
{
    NodeContainer_add(nodeset->nodes[node->nodeClass], node);
}

static void insertElementAtFront(NL_Reference **toList, NL_Reference *elem)
{
    elem->next = *toList;
    *toList = elem;
}

static bool lookupUnknownReferences(Nodeset *nodeset, NL_Node *node)
{
    while (node->unknownRefs)
    {
        NL_Reference *nextUnknown = node->unknownRefs->next;
        if (nodeset->refService->isHierachicalRef(nodeset->refService->context,
                                                  node->unknownRefs))
        {
            insertElementAtFront(&node->hierachicalRefs, node->unknownRefs);
            node->unknownRefs = nextUnknown;
            continue;
        }
        if (nodeset->refService->isNonHierachicalRef(
                nodeset->refService->context, node->unknownRefs))
        {
            insertElementAtFront(&node->nonHierachicalRefs, node->unknownRefs);
            node->unknownRefs = nextUnknown;
            continue;
        }
        return false;
    }
    return true;
}

static void lookupReferenceTypes(Nodeset *nodeset)
{
    bool allRefTypesKnown = false;
    while (!allRefTypesKnown)
    {
        allRefTypesKnown = true;
        for (size_t i = 0; i < nodeset->refTypesWithUnknownRefs->size; i++)
        {
            allRefTypesKnown = lookupUnknownReferences(
                nodeset, nodeset->refTypesWithUnknownRefs->nodes[i]);
        }
    }

    for (size_t i = 0; i < nodeset->refTypesWithUnknownRefs->size; i++)
    {
        Sort_addNode(nodeset->sortCtx,
                     nodeset->refTypesWithUnknownRefs->nodes[i]);
    }
}

bool Nodeset_sort(Nodeset *nodeset)
{
    // first we have to figure out, if there are reference types, for which we
    // cannot state if they are hierachical or nonhierachical
    lookupReferenceTypes(nodeset);
    // all hierachical references of a node should be known at this point
    // if there are nodes with unknown references, the import will be aborted
    for (size_t i = 0; i < nodeset->nodesWithUnknownRefs->size; i++)
    {
        bool result = lookupUnknownReferences(
            nodeset, nodeset->nodesWithUnknownRefs->nodes[i]);
        if (!result)
        {
            UA_String nodeIdStr = {0};
            UA_NodeId_print(&nodeset->nodesWithUnknownRefs->nodes[i]->id,
                            &nodeIdStr);
            nodeset->logger->log(
                nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                "node with unresolved reference(s): NodeId(%.*s)",
                (int)nodeIdStr.length, (char *)nodeIdStr.data);
            UA_String_clear(&nodeIdStr);
            return false;
        }
        Sort_addNode(nodeset->sortCtx, nodeset->nodesWithUnknownRefs->nodes[i]);
    }

    return Sort_start(nodeset->sortCtx, nodeset, Nodeset_addNode,
                      nodeset->logger);
}

void Nodeset_cleanup(Nodeset *nodeset)
{
    CharArenaAllocator_delete(nodeset->charArena);
    AliasList_delete(nodeset->aliasList);
    for (size_t cnt = 0; cnt < NL_NODECLASS_COUNT; cnt++)
    {
        NodeContainer_delete(nodeset->nodes[cnt]);
    }
    NodeContainer_delete(nodeset->nodesWithUnknownRefs);
    NodeContainer_delete(nodeset->refTypesWithUnknownRefs);
    NamespaceList_delete(nodeset->namespaces);
    Sort_cleanup(nodeset->sortCtx);
    NL_BiDirectionalReference *ref = nodeset->hasEncodingRefs;
    while (ref)
    {
        NL_BiDirectionalReference *tmp = ref->next;
        UA_NodeId_clear(&ref->source);
        UA_NodeId_clear(&ref->target);
        UA_NodeId_clear(&ref->refType);
        free(ref);
        ref = tmp;
    }
    free(nodeset);
}

static char *getAttributeValue(Nodeset *nodeset, const NodeAttribute *attr,
                               const char **attributes, int nb_attributes)
{
    const int fields = 5;
    for (int i = 0; i < nb_attributes; i++)
    {
        const char *localname = attributes[i * fields + 0];
        if (strcmp((const char *)localname, attr->name))
            continue;
        const char *value_start = attributes[i * fields + 3];
        const char *value_end = attributes[i * fields + 4];
        size_t size = (size_t)(value_end - value_start);
        char *value = CharArenaAllocator_malloc(nodeset->charArena, size + 1);
        memcpy(value, value_start, size);
        return value;
    }
    // we return the defaultValue, if NULL or not, following code has to cope
    // with it
    return attr->defaultValue;
}

static void extractAttributes(Nodeset *nodeset, const NamespaceList *namespaces,
                              NL_Node *node, int attributeSize,
                              const char **attributes)
{
    node->id = extractNodedId(
        namespaces,
        getAttributeValue(nodeset, &attrNodeId, attributes, attributeSize));
    node->browseName = extractBrowseName(
        namespaces,
        getAttributeValue(nodeset, &attrBrowseName, attributes, attributeSize));
    switch (node->nodeClass)
    {
    case NODECLASS_OBJECTTYPE: {
        ((NL_ObjectTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    }
    case NODECLASS_OBJECT: {
        ((NL_ObjectNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        ((NL_ObjectNode *)node)->eventNotifier = getAttributeValue(
            nodeset, &attrEventNotifier, attributes, attributeSize);
        break;
    }
    case NODECLASS_VARIABLE: {

        ((NL_VariableNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes,
                                           attributeSize);
        ((NL_VariableNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableNode *)node)->valueRank = getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize);
        ((NL_VariableNode *)node)->minimumSamplingInterval = getAttributeValue(
            nodeset, &attrMinimumSamplingInterval, attributes, attributeSize);
        ((NL_VariableNode *)node)->arrayDimensions = getAttributeValue(
            nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((NL_VariableNode *)node)->accessLevel = getAttributeValue(
            nodeset, &attrAccessLevel, attributes, attributeSize);
        ((NL_VariableNode *)node)->userAccessLevel = getAttributeValue(
            nodeset, &attrUserAccessLevel, attributes, attributeSize);
        ((NL_VariableNode *)node)->historizing = getAttributeValue(
            nodeset, &attrHistorizing, attributes, attributeSize);
        break;
    }
    case NODECLASS_VARIABLETYPE: {

        ((NL_VariableTypeNode *)node)->valueRank = getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize);
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes,
                                           attributeSize);
        ((NL_VariableTypeNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableTypeNode *)node)->arrayDimensions = getAttributeValue(
            nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((NL_VariableTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    }
    case NODECLASS_DATATYPE:
        ((NL_DataTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    case NODECLASS_METHOD:
        ((NL_MethodNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        ((NL_MethodNode *)node)->executable = getAttributeValue(
            nodeset, &attrExecutable, attributes, attributeSize);
        ((NL_MethodNode *)node)->userExecutable = getAttributeValue(
            nodeset, &attrUserExecutable, attributes, attributeSize);
        break;
    case NODECLASS_REFERENCETYPE:
        ((NL_ReferenceTypeNode *)node)->symmetric = getAttributeValue(
            nodeset, &attrSymmetric, attributes, attributeSize);
        break;
    case NODECLASS_VIEW:
        ((NL_ViewNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        ((NL_ViewNode *)node)->containsNoLoops = getAttributeValue(
            nodeset, &attrContainsNoLoops, attributes, attributeSize);
        ((NL_ViewNode *)node)->eventNotifier = getAttributeValue(
            nodeset, &attrEventNotifier, attributes, attributeSize);
        break;
    default:;
    }
}

static void initNode(Nodeset *nodeset, const NamespaceList *namespaces,
                     NL_NodeClass nodeClass, NL_Node *node, int nb_attributes,
                     const char **attributes)
{
    node->nodeClass = nodeClass;
    extractAttributes(nodeset, namespaces, node, nb_attributes, attributes);
}

NL_Node *Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                         int nb_attributes, const char **attributes)
{
    NL_Node *node = Node_new(nodeClass);
    initNode(nodeset, nodeset->namespaces, nodeClass, node, nb_attributes,
             attributes);
    return node;
}

NL_Reference *Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                                   int attributeSize, const char **attributes)
{
    NL_Reference *newRef = (NL_Reference *)calloc(1, sizeof(NL_Reference));
    if (!strcmp("true", getAttributeValue(nodeset, &attrIsForward, attributes,
                                          attributeSize)))
    {
        newRef->isForward = true;
    }
    else
    {
        newRef->isForward = false;
    }
    char *aliasIdString = getAttributeValue(nodeset, &attrReferenceType,
                                            attributes, attributeSize);

    newRef->refType = alias2Id(nodeset, aliasIdString);

    if (NODECLASS_VARIABLE == node->nodeClass &&
        nodeset->refService->isHasTypeDefRef(nodeset->refService->context,
                                             newRef))
    {
        ((NL_VariableNode *)node)->refToTypeDef = newRef;
        return newRef;
    }

    if (NODECLASS_OBJECT == node->nodeClass &&
        nodeset->refService->isHasTypeDefRef(nodeset->refService->context,
                                             newRef))
    {
        ((NL_ObjectNode *)node)->refToTypeDef = newRef;
        return newRef;
    }

    if (nodeset->refService->isHierachicalRef(nodeset->refService->context,
                                              newRef))
    {
        newRef->next = node->hierachicalRefs;
        node->hierachicalRefs = newRef;
        return newRef;
    }
    if (nodeset->refService->isNonHierachicalRef(nodeset->refService->context,
                                                 newRef))
    {
        newRef->next = node->nonHierachicalRefs;
        node->nonHierachicalRefs = newRef;
        return newRef;
    }

    newRef->next = node->unknownRefs;
    node->unknownRefs = newRef;
    return newRef;
}

Alias *Nodeset_newAlias(Nodeset *nodeset, int attributeSize,
                        const char **attributes)
{
    return AliasList_newAlias(
        nodeset->aliasList,
        getAttributeValue(nodeset, &attrAlias, attributes, attributeSize));
}

void Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString)
{
    alias->id = extractNodedId(nodeset->namespaces, idString);
}

void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext,
                                char *namespaceUri)
{
    NamespaceList_newNamespace(nodeset->namespaces, userContext, namespaceUri);
}

void Nodeset_newNodeFinish(Nodeset *nodeset, NL_Node *node)
{
    if (!node->unknownRefs)
    {
        Sort_addNode(nodeset->sortCtx, node);
        if (node->nodeClass == NODECLASS_REFERENCETYPE)
        {
            nodeset->refService->addNewReferenceType(
                nodeset->refService->context, (NL_ReferenceTypeNode *)node);
        }
    }
    else
    {
        if (node->nodeClass == NODECLASS_REFERENCETYPE)
        {
            NodeContainer_add(nodeset->refTypesWithUnknownRefs, node);
        }
        else
        {
            NodeContainer_add(nodeset->nodesWithUnknownRefs, node);
        }
    }
}

void Nodeset_newReferenceFinish(Nodeset *nodeset, NL_Reference *ref,
                                NL_Node *node, char *targetId)
{
    UA_NodeId_clear(&ref->target);
    ref->target = alias2Id(nodeset, targetId);

    // handle hasEncoding in a special way
    UA_NodeId hasEncodingRef = UA_NODEID("i=38");
    if (UA_NodeId_equal(&ref->refType, &hasEncodingRef) &&
        !strcmp(node->browseName.name, "Default Binary") && !ref->isForward)
    {
        NL_BiDirectionalReference *newRef = (NL_BiDirectionalReference *)calloc(
            1, sizeof(NL_BiDirectionalReference));
        UA_NodeId_copy(&ref->target, &newRef->source);
        UA_NodeId_copy(&node->id, &newRef->target);
        UA_NodeId_copy(&ref->refType, &newRef->refType);

        NL_BiDirectionalReference *lastRef = nodeset->hasEncodingRefs;
        nodeset->hasEncodingRefs = newRef;
        newRef->next = lastRef;
    }
}

void Nodeset_addDataTypeDefinition(Nodeset *nodeset, NL_Node *node,
                                   int attributeSize, const char **attributes)
{
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    NL_DataTypeDefinition *def = DataTypeDefinition_new(dataTypeNode);
    def->isUnion =
        !strcmp("true", getAttributeValue(nodeset, &dataTypeDefinition_IsUnion,
                                          attributes, attributeSize));
    def->isOptionSet = !strcmp(
        "true", getAttributeValue(nodeset, &dataTypeDefinition_IsOptionSet,
                                  attributes, attributeSize));
}

void Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              int attributeSize, const char **attributes)
{
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if (dataTypeNode->definition->isOptionSet)
    {
        return;
    }

    NL_DataTypeDefinitionField *newField =
        DataTypeNode_addDefinitionField(dataTypeNode->definition);
    newField->name = getAttributeValue(nodeset, &dataTypeField_Name, attributes,
                                       attributeSize);

    char *value = getAttributeValue(nodeset, &dataTypeField_Value, attributes,
                                    attributeSize);
    if (value)
    {
        newField->value = atoi(value);
        dataTypeNode->definition->isEnum =
            !dataTypeNode->definition->isOptionSet;
    }
    else
    {
        newField->dataType = alias2Id(
            nodeset, getAttributeValue(nodeset, &dataTypeField_DataType,
                                       attributes, attributeSize));
        newField->valueRank = atoi(getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize));
        char *isOptional = getAttributeValue(nodeset, &dataTypeField_IsOptional,
                                             attributes, attributeSize);
        newField->isOptional = !strcmp("true", isOptional);
    }
}

const NL_BiDirectionalReference *
Nodeset_getBiDirectionalRefs(const Nodeset *nodeset)
{
    return nodeset->hasEncodingRefs;
}

void Nodeset_setDisplayName(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes)
{
    node->displayName.locale =
        getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
}

void Nodeset_DisplayNameFinish(const Nodeset *nodeset, NL_Node *node,
                               char *text)
{
    node->displayName.text = text;
}

void Nodeset_setDescription(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes)
{
    node->description.locale =
        getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
}

void Nodeset_DescriptionFinish(const Nodeset *nodeset, NL_Node *node,
                               char *text)
{
    node->description.text = text;
}

void Nodeset_setInverseName(Nodeset *nodeset, NL_Node *node, int attributeSize,
                            const char **attributes)
{
    if (node->nodeClass == NODECLASS_REFERENCETYPE)
    {
        ((NL_ReferenceTypeNode *)node)->inverseName.locale =
            getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
    }
}
void Nodeset_InverseNameFinish(const Nodeset *nodeset, NL_Node *node,
                               char *text)
{
    if (node->nodeClass == NODECLASS_REFERENCETYPE)
    {
        ((NL_ReferenceTypeNode *)node)->inverseName.text = text;
    }
}

size_t Nodeset_forEachNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                           void *context,
                           NodesetLoader_forEachNode_Func fn)
{
    NodeContainer *c = nodeset->nodes[nodeClass];
    for (NL_Node **node = c->nodes; node != c->nodes + c->size; node++)
    {
        fn(context, *node);
    }
    return c->size;
}
