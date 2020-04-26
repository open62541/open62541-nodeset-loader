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
const NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, NULL};
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
const NodeAttribute dataTypeField_DataType = {"DataType", NULL};
const NodeAttribute dataTypeField_Value = {"Value", NULL};

TReferenceTypeNode hierachicalRefs[MAX_HIERACHICAL_REFS] = {
    {NODECLASS_REFERENCETYPE,
     {0, "i=35"},
     {0, "Organizes"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=36"},
     {0, "HasEventSource"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=48"},
     {0, "HasNotifier"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=44"},
     {0, "Aggregates"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=45"},
     {0, "HasSubtype"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=47"},
     {0, "HasComponent"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=46"},
     {0, "HasProperty"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, "i=47"},
     {0, "HasEncoding"},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
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

Nodeset *Nodeset_new(addNamespaceCb nsCallback)
{
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));

    nodeset->aliasList = AliasList_new();
    nodeset->namespaces = NamespaceList_new(nsCallback);
    nodeset->charArena = CharArenaAllocator_new(1024 * 1024 * 20);
    nodeset->nodes[NODECLASS_OBJECT] = NodeContainer_new(10000);
    nodeset->nodes[NODECLASS_VARIABLE] = NodeContainer_new(10000);
    nodeset->nodes[NODECLASS_METHOD] = NodeContainer_new(1000);
    nodeset->nodes[NODECLASS_OBJECTTYPE] = NodeContainer_new(100);
    nodeset->nodes[NODECLASS_DATATYPE] = NodeContainer_new(100);
    nodeset->nodes[NODECLASS_REFERENCETYPE] = NodeContainer_new(100);
    nodeset->nodes[NODECLASS_VARIABLETYPE] = NodeContainer_new(100);
    // known hierachical refs
    nodeset->hierachicalRefs = hierachicalRefs;
    nodeset->hierachicalRefsSize = 8;
    nodeset->sortCtx = Sort_init();
    return nodeset;
}

static void Nodeset_addNode(Nodeset *nodeset, TNode *node)
{
    NodeContainer_add(nodeset->nodes[node->nodeClass], node);
}

bool Nodeset_sort(Nodeset *nodeset)
{
    return Sort_start(nodeset->sortCtx, nodeset, Nodeset_addNode);
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
    NamespaceList_delete(nodeset->namespaces);
    Sort_cleanup(nodeset->sortCtx);
    free(nodeset);
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
    case NODECLASS_DATATYPE:;
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
    node->hierachicalRefs = NULL;
    node->nonHierachicalRefs = NULL;
    node->displayName = NULL;
    node->description = NULL;
    node->writeMask = NULL;
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

static bool isKnownReferenceType(Nodeset *nodeset, const TNodeId *refTypeId)
{
    // we state that we know all references from namespace 0
    if (refTypeId->nsIdx == 0)
    {
        return true;
    }
    for (size_t i = 0; i < nodeset->nodes[NODECLASS_REFERENCETYPE]->size; i++)
    {
        if (!TNodeId_cmp(
                refTypeId,
                &nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes[i]->id))
        {
            return true;
        }
    }
    return false;
}

Reference *Nodeset_newReference(Nodeset *nodeset, TNode *node,
                                int attributeSize, const char **attributes)
{
    Reference *newRef = (Reference *)malloc(sizeof(Reference));
    newRef->target.id = NULL;
    newRef->refType.id = NULL;
    newRef->next = NULL;
    if (!strcmp("true", getAttributeValue(nodeset, &attrIsForward, attributes,
                                          attributeSize)))
    {
        newRef->isForward = true;
    }
    else
    {
        newRef->isForward = false;
    }
    // TODO: should we check if its an alias
    char *aliasIdString = getAttributeValue(nodeset, &attrReferenceType,
                                            attributes, attributeSize);

    newRef->refType = alias2Id(nodeset, aliasIdString);

    bool isKnownRef = isKnownReferenceType(nodeset, &newRef->refType);
    // TODO: we have to check later on, if it's really a hierachical reference
    // type, otherwise the reference should be marked as non hierachical
    if (isHierachicalReference(nodeset, newRef) || !isKnownRef)
    {
        Reference *lastRef = node->hierachicalRefs;
        node->hierachicalRefs = newRef;
        newRef->next = lastRef;
    }
    else
    {
        Reference *lastRef = node->nonHierachicalRefs;
        node->nonHierachicalRefs = newRef;
        newRef->next = lastRef;
    }
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

static void addIfHierachicalReferenceType(Nodeset *nodeset, TNode *node)
{
    Reference *ref = node->hierachicalRefs;
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
                    break;
                }
            }
        }
        ref = ref->next;
    }
}

void Nodeset_newNodeFinish(Nodeset *nodeset, TNode *node)
{
    Sort_addNode(nodeset->sortCtx, node);
    if (node->nodeClass == NODECLASS_REFERENCETYPE)
    {
        addIfHierachicalReferenceType(nodeset, node);
    }
}

void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, TNode *node,
                                char *targetId)
{
    ref->target = extractNodedId(nodeset->namespaces, targetId);
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
