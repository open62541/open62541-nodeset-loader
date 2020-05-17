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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool isHierachicalReference(Nodeset *nodeset, const Reference *ref);
static TNodeId extractNodedId(const NamespaceList *namespaces, char *s);
static TNodeId alias2Id(const Nodeset *nodeset, char *name);
static TNodeId translateNodeId(const NamespaceList *namespaces, TNodeId id);
static TBrowseName translateBrowseName(const NamespaceList *namespaces,
                                       TBrowseName id);
TBrowseName extractBrowseName(const NamespaceList *namespaces, char *s);

#define MAX_HIERACHICAL_REFS 50

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
// UAObject
#define ATTRIBUTE_EVENTNOTIFIER "EventNotifier"
// UAObjectType
#define ATTRIBUTE_ISABSTRACT "IsAbstract"
// Reference
#define ATTRIBUTE_REFERENCETYPE "ReferenceType"
#define ATTRIBUTE_ISFORWARD "IsForward"
#define ATTRIBUTE_SYMMETRIC "Symmetric"
#define ATTRIBUTE_ALIAS "Alias"

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
const NodeAttribute dataTypeField_Name = {"Name", NULL};
const NodeAttribute dataTypeField_DataType = {"DataType", "i=24"};
const NodeAttribute dataTypeField_Value = {"Value", NULL};
const NodeAttribute attrLocale = {"Locale", NULL};
const NodeAttribute attrHistorizing = {ATTRIBUTE_HISTORIZING, "false"};

TReferenceTypeNode hierachicalRefs[MAX_HIERACHICAL_REFS] = {
    {
        NODECLASS_REFERENCETYPE,
        {0, "i=35"},
        {0, "Organizes"},
        {NULL, NULL},
        {NULL, NULL},
        NULL,
        NULL,
        NULL,
        NULL,
        {NULL, NULL},
        NULL,
    },
    {NODECLASS_REFERENCETYPE,
     {0, "i=36"},
     {0, "HasEventSource"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=48"},
     {0, "HasNotifier"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=44"},
     {0, "Aggregates"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=45"},
     {0, "HasSubtype"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=47"},
     {0, "HasComponent"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=46"},
     {0, "HasProperty"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=47"},
     {0, "HasEncoding"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=33"},
     {0, "HasEncoding"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
};

TNodeId translateNodeId(const NamespaceList *namespaces, TNodeId id)
{
    if (id.nsIdx > 0)
    {
        id.nsIdx = (int)NamespaceList_getNamespace(namespaces, id.nsIdx)->idx;
        return id;
    }
    return id;
}

TBrowseName translateBrowseName(const NamespaceList *namespaces, TBrowseName bn)
{
    if (bn.nsIdx > 0)
    {
        bn.nsIdx =
            (uint16_t)NamespaceList_getNamespace(namespaces, bn.nsIdx)->idx;
        return bn;
    }
    return bn;
}

TNodeId extractNodedId(const NamespaceList *namespaces, char *s)
{
    if (s == NULL)
    {
        TNodeId id;
        id.id = NULL;
        id.nsIdx = 0;
        return id;
    }
    TNodeId id;
    id.nsIdx = 0;
    char *idxSemi = strchr(s, ';');
    if (idxSemi == NULL)
    {
        id.id = s;
        return id;
    }
    else
    {
        id.nsIdx = atoi(&s[3]);
        id.id = idxSemi + 1;
    }
    return translateNodeId(namespaces, id);
}

TBrowseName extractBrowseName(const NamespaceList *namespaces, char *s)
{
    TBrowseName bn;
    bn.nsIdx = 0;
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

static TNodeId alias2Id(const Nodeset *nodeset, char *name)
{
    const TNodeId *alias = AliasList_getNodeId(nodeset->aliasList, name);
    if (!alias)
    {
        return extractNodedId(nodeset->namespaces, name);
    }
    return *alias;
}

Nodeset *Nodeset_new(addNamespaceCb nsCallback, NodesetLoader_Logger *logger)
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
    nodeset->nodesWithUnknownRefs = NodeContainer_new(100, false);
    nodeset->refTypesWithUnknownRefs = NodeContainer_new(100, false);
    nodeset->nonHierachicalRefs = NodeContainer_new(100, false);
    // known hierachical refs
    nodeset->hierachicalRefs = hierachicalRefs;
    nodeset->hierachicalRefsSize = 9;
    nodeset->sortCtx = Sort_init();
    nodeset->logger = logger;
    return nodeset;
}

static void addToRefTypes(Nodeset *nodeset, TNode *node)
{
    Reference *ref = node->hierachicalRefs;
    bool isHierachical = false;
    while (ref)
    {
        if (!ref->isForward)
        {
            for (size_t i = 0; i < nodeset->hierachicalRefsSize; i++)
            {
                if (!TNodeId_cmp(&nodeset->hierachicalRefs[i].id, &ref->target))
                {
                    nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] =
                        *(TReferenceTypeNode *)node;
                    isHierachical = true;
                    break;
                }
            }
        }
        ref = ref->next;
    }
    if (!isHierachical)
    {
        NodeContainer_add(nodeset->nonHierachicalRefs, node);
    }
}
static bool isNonHierachicalRef(Nodeset *nodeset, const Reference *ref)
{
    // TODO: nonHierachicalrefs should also be imported first
    // we state that we know all references from namespace 0
    if (ref->refType.nsIdx == 0)
    {
        return true;
    }
    for (size_t i = 0; i < nodeset->nonHierachicalRefs->size; i++)
    {
        if (!TNodeId_cmp(&ref->refType,
                         &nodeset->nonHierachicalRefs->nodes[i]->id))
        {
            return true;
        }
    }
    return false;
}

static bool isHierachicalReference(Nodeset *nodeset, const Reference *ref)
{
    for (size_t i = 0; i < nodeset->hierachicalRefsSize; i++)
    {
        if (!TNodeId_cmp(&ref->refType, &nodeset->hierachicalRefs[i].id))
        {
            return true;
        }
    }
    return false;
}

static void Nodeset_addNode(Nodeset *nodeset, TNode *node)
{
    NodeContainer_add(nodeset->nodes[node->nodeClass], node);
}

static bool lookupUnknownReferences(Nodeset *nodeset, TNode *node)
{
    while (node->unknownRefs)
    {
        Reference *next = node->unknownRefs->next;
        if (isHierachicalReference(nodeset, node->unknownRefs))
        {
            node->unknownRefs->next = node->hierachicalRefs;
            node->hierachicalRefs = node->unknownRefs;
            node->unknownRefs = next;
            continue;
        }
        if (isNonHierachicalRef(nodeset, node->unknownRefs))
        {
            node->unknownRefs->next = node->nonHierachicalRefs;
            node->nonHierachicalRefs = node->unknownRefs;
            node->unknownRefs = next;
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
    // all hierachical should be known at this point
    for (size_t i = 0; i < nodeset->nodesWithUnknownRefs->size; i++)
    {
        bool result = lookupUnknownReferences(
            nodeset, nodeset->nodesWithUnknownRefs->nodes[i]);
        if(!result)
        {
            nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR, "node with unresolved reference");
        }
        assert(result);
        Sort_addNode(nodeset->sortCtx, nodeset->nodesWithUnknownRefs->nodes[i]);
    }

    return Sort_start(nodeset->sortCtx, nodeset, Nodeset_addNode,
                      nodeset->logger);
}

size_t Nodeset_getNodes(Nodeset *nodeset, TNodeClass nodeClass, TNode ***nodes)
{
    *nodes = nodeset->nodes[nodeClass]->nodes;
    return nodeset->nodes[nodeClass]->size;
}

void Nodeset_cleanup(Nodeset *nodeset)
{
    CharArenaAllocator_delete(nodeset->charArena);
    AliasList_delete(nodeset->aliasList);
    for (size_t cnt = 0; cnt < NODECLASS_COUNT; cnt++)
    {
        NodeContainer_delete(nodeset->nodes[cnt]);
    }
    NodeContainer_delete(nodeset->nodesWithUnknownRefs);
    NodeContainer_delete(nodeset->nonHierachicalRefs);
    NodeContainer_delete(nodeset->refTypesWithUnknownRefs);
    NamespaceList_delete(nodeset->namespaces);
    Sort_cleanup(nodeset->sortCtx);
    BiDirectionalReference *ref = nodeset->hasEncodingRefs;
    while (ref)
    {
        BiDirectionalReference *tmp = ref->next;
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
                              TNode *node, int attributeSize,
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
    case NODECLASS_OBJECTTYPE:
    {
        ((TObjectTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    }
    case NODECLASS_OBJECT:
    {
        ((TObjectNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        ((TObjectNode *)node)->eventNotifier = getAttributeValue(
            nodeset, &attrEventNotifier, attributes, attributeSize);
        break;
    }
    case NODECLASS_VARIABLE:
    {

        ((TVariableNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes,
                                           attributeSize);
        ((TVariableNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((TVariableNode *)node)->valueRank = getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize);
        ((TVariableNode *)node)->arrayDimensions = getAttributeValue(
            nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((TVariableNode *)node)->accessLevel = getAttributeValue(
            nodeset, &attrAccessLevel, attributes, attributeSize);
        ((TVariableNode *)node)->userAccessLevel = getAttributeValue(
            nodeset, &attrUserAccessLevel, attributes, attributeSize);
        ((TVariableNode *)node)->historizing = getAttributeValue(
            nodeset, &attrHistorizing, attributes, attributeSize);
        break;
    }
    case NODECLASS_VARIABLETYPE:
    {

        ((TVariableTypeNode *)node)->valueRank = getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize);
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes,
                                           attributeSize);
        ((TVariableTypeNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((TVariableTypeNode *)node)->arrayDimensions = getAttributeValue(
            nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((TVariableTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    }
    case NODECLASS_DATATYPE:
        ((TDataTypeNode *)node)->isAbstract = getAttributeValue(
            nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    case NODECLASS_METHOD:
        ((TMethodNode *)node)->parentNodeId = extractNodedId(
            namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                          attributes, attributeSize));
        ((TMethodNode *)node)->executable = getAttributeValue(
            nodeset, &attrExecutable, attributes, attributeSize);
        ((TMethodNode *)node)->userExecutable = getAttributeValue(
            nodeset, &attrUserExecutable, attributes, attributeSize);
        break;
    case NODECLASS_REFERENCETYPE:
        ((TReferenceTypeNode *)node)->symmetric = getAttributeValue(
            nodeset, &attrSymmetric, attributes, attributeSize);
        break;
    default:;
    }
}

static void initNode(Nodeset *nodeset, const NamespaceList *namespaces,
                     TNodeClass nodeClass, TNode *node, int nb_attributes,
                     const char **attributes)
{
    node->nodeClass = nodeClass;
    extractAttributes(nodeset, namespaces, node, nb_attributes, attributes);
}

TNode *Nodeset_newNode(Nodeset *nodeset, TNodeClass nodeClass,
                       int nb_attributes, const char **attributes)
{
    TNode *node = Node_new(nodeClass);
    initNode(nodeset, nodeset->namespaces, nodeClass, node, nb_attributes,
             attributes);
    return node;
}

Reference *Nodeset_newReference(Nodeset *nodeset, TNode *node,
                                int attributeSize, const char **attributes)
{
    Reference *newRef = (Reference *)calloc(1, sizeof(Reference));
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
        !strcmp("i=40", newRef->refType.id))
    {
        ((TVariableNode *)node)->refToTypeDef = newRef;
        return newRef;
    }

    if (NODECLASS_OBJECT == node->nodeClass &&
        !strcmp("i=40", newRef->refType.id))
    {
        ((TObjectNode *)node)->refToTypeDef = newRef;
        return newRef;
    }

    if (isHierachicalReference(nodeset, newRef))
    {
        newRef->next = node->hierachicalRefs;
        node->hierachicalRefs = newRef;
        return newRef;
    }
    if (isNonHierachicalRef(nodeset, newRef))
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

void Nodeset_newNodeFinish(Nodeset *nodeset, TNode *node)
{
    if (!node->unknownRefs)
    {
        Sort_addNode(nodeset->sortCtx, node);
        if (node->nodeClass == NODECLASS_REFERENCETYPE)
        {
            addToRefTypes(nodeset, node);
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

void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, TNode *node,
                                char *targetId)
{
    ref->target = alias2Id(nodeset, targetId);
    // handle hasEncoding in a special way
    TNodeId hasEncodingRef = {0, "i=38"};
    if (!TNodeId_cmp(&ref->refType, &hasEncodingRef) &&
        !strcmp(node->browseName.name, "Default Binary") && !ref->isForward)
    {
        BiDirectionalReference *newRef =
            (BiDirectionalReference *)calloc(1, sizeof(BiDirectionalReference));
        newRef->source = ref->target;
        newRef->target = node->id;
        newRef->refType = ref->refType;

        BiDirectionalReference *lastRef = nodeset->hasEncodingRefs;
        nodeset->hasEncodingRefs = newRef;
        newRef->next = lastRef;
    }
}

void Nodeset_addDataTypeField(Nodeset *nodeset, TNode *node, int attributeSize,
                              const char **attributes)
{
    TDataTypeNode *dataTypeNode = (TDataTypeNode *)node;

    DataTypeDefinitionField *newField =
        DataTypeNode_addDefinitionField(dataTypeNode);
    newField->name = getAttributeValue(nodeset, &dataTypeField_Name, attributes,
                                       attributeSize);

    char *value = getAttributeValue(nodeset, &dataTypeField_Value, attributes,
                                    attributeSize);
    if (value)
    {
        newField->value = atoi(value);
        dataTypeNode->definition->isEnum = true;
    }
    else
    {
        newField->dataType = alias2Id(
            nodeset, getAttributeValue(nodeset, &dataTypeField_DataType,
                                       attributes, attributeSize));
        newField->valueRank = atoi(getAttributeValue(
            nodeset, &attrValueRank, attributes, attributeSize));
    }
}

const BiDirectionalReference *
Nodeset_getBiDirectionalRefs(const Nodeset *nodeset)
{
    return nodeset->hasEncodingRefs;
}

void Nodeset_setDisplayName(Nodeset *nodeset, TNode *node, int attributeSize,
                            const char **attributes)
{
    node->displayName.locale =
        getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
}

void Nodeset_DisplayNameFinish(const Nodeset *nodeset, TNode *node, char *text)
{
    node->displayName.text = text;
}

void Nodeset_setDescription(Nodeset *nodeset, TNode *node, int attributeSize,
                            const char **attributes)
{
    node->description.locale =
        getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
}

void Nodeset_DescriptionFinish(const Nodeset *nodeset, TNode *node, char *text)
{
    node->description.text = text;
}

void Nodeset_setInverseName(Nodeset *nodeset, TNode *node, int attributeSize,
                            const char **attributes)
{
    if (node->nodeClass == NODECLASS_REFERENCETYPE)
    {
        ((TReferenceTypeNode *)node)->inverseName.locale =
            getAttributeValue(nodeset, &attrLocale, attributes, attributeSize);
    }
}
void Nodeset_InverseNameFinish(const Nodeset *nodeset, TNode *node, char *text)
{
    if (node->nodeClass == NODECLASS_REFERENCETYPE)
    {
        ((TReferenceTypeNode *)node)->inverseName.text = text;
    }
}
