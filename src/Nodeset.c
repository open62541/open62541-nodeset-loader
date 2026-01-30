/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "Nodeset.h"
#include "AliasList.h"
#include "Node.h"
#include <open62541/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATTRIBUTE_NODEID "NodeId"
#define ATTRIBUTE_BROWSENAME "BrowseName"
#define ATTRIBUTE_PARENTNODEID "ParentNodeId"
#define ATTRIBUTE_DATATYPE "DataType"
#define ATTRIBUTE_VALUERANK "ValueRank"
#define ATTRIBUTE_ARRAYDIMENSIONS "ArrayDimensions"
#define ATTRIBUTE_HISTORIZING "Historizing"
#define ATTRIBUTE_MINIMUMSAMPLINGINTERVAL "MinimumSamplingInterval"
#define ATTRIBUTE_EVENTNOTIFIER "EventNotifier"
#define ATTRIBUTE_ISABSTRACT "IsAbstract"
#define ATTRIBUTE_REFERENCETYPE "ReferenceType"
#define ATTRIBUTE_ISFORWARD "IsForward"
#define ATTRIBUTE_SYMMETRIC "Symmetric"
#define ATTRIBUTE_ALIAS "Alias"
#define ATTRIBUTE_CONTAINSNOLOOPS "ContainsNoLoops"

typedef struct {
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

static UA_NodeId
parseNodeId(const Nodeset *nodeset, char *s) {
    UA_NodeId n;
    UA_NodeId_parseEx(&n, UA_STRING(s), &nodeset->fc->nsMapping);
    return n;
}

static UA_QualifiedName
parseQualifiedName(const Nodeset *nodeset, char *s) {
    UA_QualifiedName qn;
    UA_QualifiedName_parseEx(&qn, UA_STRING(s), &nodeset->fc->nsMapping);
    qn.namespaceIndex = UA_NamespaceMapping_remote2Local(&nodeset->fc->nsMapping, qn.namespaceIndex);
    return qn;
}

static UA_NodeId
alias2Id(const Nodeset *nodeset, char *name) {
    const UA_NodeId *alias = AliasList_getNodeId(nodeset->aliasList, name);
    if(!alias)
        return parseNodeId(nodeset, name);
    return *alias;
}

Nodeset *
Nodeset_new(NL_addNamespaceCallback nsCallback,
            NodesetLoader_Logger *logger) {
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));
    if(!nodeset)
        return NULL;

    nodeset->aliasList = AliasList_new();
    nodeset->charArena = CharArenaAllocator_new(1024 * 1024);
    NodeContainer_init(&nodeset->nodes[NODECLASS_OBJECT], 1000);
    NodeContainer_init(&nodeset->nodes[NODECLASS_VARIABLE], 1000);
    NodeContainer_init(&nodeset->nodes[NODECLASS_METHOD], 1000);
    NodeContainer_init(&nodeset->nodes[NODECLASS_OBJECTTYPE], 100);
    NodeContainer_init(&nodeset->nodes[NODECLASS_DATATYPE], 100);
    NodeContainer_init(&nodeset->nodes[NODECLASS_REFERENCETYPE], 100);
    NodeContainer_init(&nodeset->nodes[NODECLASS_VARIABLETYPE], 100);
    NodeContainer_init(&nodeset->nodes[NODECLASS_VIEW], 10);
    NodeContainer_init(&nodeset->allNodes, 10000);
    NodeContainer_init(&nodeset->sortedNodes, 10000);
    nodeset->logger = logger;
    return nodeset;
}

static int
compareNodeByNodeId(const void *a, const void *b) {
    const NL_Node *na = *(const NL_Node * const *)a;
    const NL_Node *nb = *(const NL_Node * const *)b;
    return UA_NodeId_order(&na->id, &nb->id);
}

// Search in ns->allNodes, but sort before!
static NL_Node *
Nodeset_findByNodeId(Nodeset *nodeset, const UA_NodeId *key) {
    size_t left = 0;
    size_t right = nodeset->allNodes.size;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        NL_Node *node = nodeset->allNodes.nodes[mid];
        UA_Order ord = UA_NodeId_order(&node->id, key);
        if(ord == UA_ORDER_EQ)
            return node;
        if (ord == UA_ORDER_LESS)
            left = mid + 1;
        else
            right = mid;
    }
    return NULL;
}

static UA_NodeId hasTypeDef = {0, UA_NODEIDTYPE_NUMERIC, {40}};

static bool
nodeRefsReady(NL_Node *node) {
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        if(!ref->targetPtr)
            continue;
        if(ref->targetPtr->isDone)
            continue;
        if(UA_NodeId_equal(&hasTypeDef, &ref->refType)) {
            if(ref->isForward)
                return false;
        } else {
            if(!ref->isForward)
                return false;
        }
    }
    return true;
}

// Returns true if all nodes could be added
static bool
Nodeset_sortNodeClass(Nodeset *nodeset, NL_NodeClass nodeClass) {
    NodeContainer *nc = &nodeset->nodes[nodeClass];
    size_t oldSize;

    // Check all nodes if they can be inserted now.
    // Retry until all nodes have been added or a fixpoint was reached.
 retry:
    oldSize = nc->size;
    for(size_t i = 0; i < nc->size; i++) {
        NL_Node *node = nc->nodes[i];
        if(!nodeRefsReady(node))
            continue;
        NodeContainer_add(&nodeset->sortedNodes, node);
        NodeContainer_remove(nc, i);
        i--;
        node->isDone = true;
    }

    if(oldSize != nc->size)
        goto retry;

    return (nc->size == 0);
}

bool Nodeset_sort(Nodeset *nodeset) {
    // Make allNodes a sorted list
    qsort(nodeset->allNodes.nodes, nodeset->allNodes.size,
          sizeof(NL_Node *), compareNodeByNodeId);

    // Insert a pointer to the target node for all references.
    // If the target is not found in allNodes, assume it already exists in the server.
    for(size_t i = 0; i < nodeset->allNodes.size; i++) {
        NL_Node *node = nodeset->allNodes.nodes[i];
        for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
            ref->targetPtr = Nodeset_findByNodeId(nodeset, &ref->target);
        }
    }

    // Add ReferenceTypes
    bool done = Nodeset_sortNodeClass(nodeset, NODECLASS_REFERENCETYPE);
    if(!done) {
        nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                             "Cannot add ReferenceType hierarchy");
    }

    // Add DataTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_DATATYPE);
    if(!done) {
        nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                             "Cannot add DataType hierarchy");
    }

    // Add VariableTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLETYPE);
    if(!done) {
        nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                             "Cannot add VariableType hierarchy");
    }

    // Add Views
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VIEW);
    if(!done) {
        nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                             "Cannot add Views");
    }

    // Add ObjectType, Object, Method and Variable
    size_t totalSorted;
 retry:
    totalSorted = nodeset->sortedNodes.size;
    done = true;
    done |= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECTTYPE);
    done |= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECT);
    done |= Nodeset_sortNodeClass(nodeset, NODECLASS_METHOD);
    done |= Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLE);
    if(done)
        goto finish;
    if(totalSorted == nodeset->sortedNodes.size) {
        nodeset->logger->log(nodeset->logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                             "Infinite loop in the references");
        goto finish;
    }
    goto retry;

 finish:
    // Set isDone to false again
    for(size_t i = 0; i < nodeset->allNodes.size; i++) {
        NL_Node *node = nodeset->allNodes.nodes[i];
        node->isDone = false;
    }
    return done;
}

void Nodeset_cleanup(Nodeset *nodeset) {
    CharArenaAllocator_delete(nodeset->charArena);
    AliasList_delete(nodeset->aliasList);
    for (size_t cnt = 0; cnt < NL_NODECLASS_COUNT; cnt++) {
        NodeContainer_clear(&nodeset->nodes[cnt]);
    }
    NodeContainer_clear(&nodeset->allNodes);
    NodeContainer_clear(&nodeset->sortedNodes);
    free(nodeset);
}

static char *
getAttributeValue(Nodeset *nodeset, const NodeAttribute *attr,
                               const char **attributes, size_t nb_attributes) {
    const size_t fields = 5;
    for (size_t i = 0; i < nb_attributes; i++) {
        const char *localname = attributes[i * fields + 0];
        if(strcmp((const char *)localname, attr->name))
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

static void
extractAttributes(Nodeset *nodeset, NL_Node *node,
                  size_t attributeSize, const char **attributes) {
    node->id =
        parseNodeId(nodeset, getAttributeValue(nodeset, &attrNodeId,
                                               attributes, attributeSize));
    node->browseName =
        parseQualifiedName(nodeset, getAttributeValue(nodeset, &attrBrowseName,
                                                      attributes, attributeSize));
    switch (node->nodeClass) {
    case NODECLASS_OBJECTTYPE:
        ((NL_ObjectTypeNode *)node)->isAbstract =
            getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize);
        break;

    case NODECLASS_OBJECT:
        ((NL_ObjectNode *)node)->parentNodeId =
            parseNodeId(nodeset, getAttributeValue(nodeset, &attrParentNodeId,
                                                      attributes, attributeSize));
        ((NL_ObjectNode *)node)->eventNotifier =
            getAttributeValue(nodeset, &attrEventNotifier, attributes, attributeSize);
        break;

    case NODECLASS_VARIABLE: {
        ((NL_VariableNode *)node)->parentNodeId =
            parseNodeId(nodeset, getAttributeValue(nodeset, &attrParentNodeId,
                                                      attributes, attributeSize));
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes,
                                           attributeSize);
        ((NL_VariableNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableNode *)node)->valueRank =
            getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize);
        ((NL_VariableNode *)node)->minimumSamplingInterval =
            getAttributeValue(nodeset, &attrMinimumSamplingInterval, attributes, attributeSize);
        ((NL_VariableNode *)node)->arrayDimensions =
            getAttributeValue(nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((NL_VariableNode *)node)->accessLevel =
            getAttributeValue(nodeset, &attrAccessLevel, attributes, attributeSize);
        ((NL_VariableNode *)node)->userAccessLevel =
            getAttributeValue(nodeset, &attrUserAccessLevel, attributes, attributeSize);
        ((NL_VariableNode *)node)->historizing =
            getAttributeValue(nodeset, &attrHistorizing, attributes, attributeSize);
        break;
    }

    case NODECLASS_VARIABLETYPE: {
        ((NL_VariableTypeNode *)node)->valueRank =
            getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize);
        char *datatype = getAttributeValue(nodeset, &attrDataType, attributes, attributeSize);
        ((NL_VariableTypeNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableTypeNode *)node)->arrayDimensions =
            getAttributeValue(nodeset, &attrArrayDimensions, attributes, attributeSize);
        ((NL_VariableTypeNode *)node)->isAbstract =
            getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize);
        break;
    }

    case NODECLASS_DATATYPE:
        ((NL_DataTypeNode *)node)->isAbstract =
            getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize);
        break;

    case NODECLASS_METHOD:
        ((NL_MethodNode *)node)->parentNodeId =
            parseNodeId(nodeset, getAttributeValue(nodeset, &attrParentNodeId,
                                                      attributes, attributeSize));
        ((NL_MethodNode *)node)->executable =
            getAttributeValue(nodeset, &attrExecutable, attributes, attributeSize);
        ((NL_MethodNode *)node)->userExecutable =
            getAttributeValue(nodeset, &attrUserExecutable, attributes, attributeSize);
        break;

    case NODECLASS_REFERENCETYPE:
        ((NL_ReferenceTypeNode *)node)->symmetric =
            getAttributeValue(nodeset, &attrSymmetric, attributes, attributeSize);
        break;

    case NODECLASS_VIEW:
        ((NL_ViewNode *)node)->parentNodeId =
            parseNodeId(nodeset, getAttributeValue(nodeset, &attrParentNodeId,
                                                      attributes, attributeSize));
        ((NL_ViewNode *)node)->containsNoLoops =
            getAttributeValue(nodeset, &attrContainsNoLoops, attributes, attributeSize);
        ((NL_ViewNode *)node)->eventNotifier =
            getAttributeValue(nodeset, &attrEventNotifier, attributes, attributeSize);
        break;

    default:
        break;
    }
}

NL_Node *
Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                size_t nb_attributes, const char **attributes) {
    NL_Node *node = Node_new(nodeClass);
    node->nodeClass = nodeClass;
    extractAttributes(nodeset, node, nb_attributes, attributes);
    NodeContainer_add(&nodeset->nodes[node->nodeClass], node);
    NodeContainer_add(&nodeset->allNodes, node);
    return node;
}

NL_Reference *
Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                     size_t attributeSize, const char **attributes) {
    NL_Reference *newRef = (NL_Reference *)calloc(1, sizeof(NL_Reference));

    char *isForwardString =
        getAttributeValue(nodeset, &attrIsForward, attributes, attributeSize);
    if(!strcmp("true", isForwardString)) {
        newRef->isForward = true;
    } else {
        newRef->isForward = false;
    }

    char *aliasIdString =
        getAttributeValue(nodeset, &attrReferenceType, attributes, attributeSize);
    newRef->refType = alias2Id(nodeset, aliasIdString);

    newRef->next = node->refs;
    node->refs = newRef;
    return newRef;
}

void
Nodeset_newReference_finish(Nodeset *nodeset, NL_Reference *ref,
                            char *idString) {
    ref->target = alias2Id(nodeset, idString);
}

Alias *
Nodeset_newAlias(Nodeset *nodeset, size_t attributeSize, const char **attributes) {
    return AliasList_newAlias(nodeset->aliasList,
                              getAttributeValue(nodeset, &attrAlias,
                                                attributes, attributeSize));
}

void
Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString) {
    alias->id = parseNodeId(nodeset, idString);
}

void
Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext,
                           char *namespaceUri) {
    UA_String uri = UA_STRING(namespaceUri);
    UA_StatusCode ret =
        UA_Array_appendCopy((void**)&nodeset->localNamespaceUris,
                            &nodeset->localNamespaceUrisSize,
                            &uri, &UA_TYPES[UA_TYPES_STRING]);
    (void)ret;
    nodeset->fc->addNamespace(nodeset->fc->userContext,
                              nodeset->localNamespaceUrisSize,
                              nodeset->localNamespaceUris,
                              &nodeset->fc->nsMapping);
}

void Nodeset_addDataTypeDefinition(Nodeset *nodeset, NL_Node *node,
                                   size_t attributeSize, const char **attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    NL_DataTypeDefinition *def = DataTypeDefinition_new(dataTypeNode);
    def->isUnion =
        !strcmp("true", getAttributeValue(nodeset, &dataTypeDefinition_IsUnion,
                                          attributes, attributeSize));
    def->isOptionSet =
        !strcmp("true", getAttributeValue(nodeset, &dataTypeDefinition_IsOptionSet,
                                          attributes, attributeSize));
}

void Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              size_t attributeSize, const char **attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if(dataTypeNode->definition->isOptionSet)
        return;

    NL_DataTypeDefinitionField *newField =
        DataTypeNode_addDefinitionField(dataTypeNode->definition);
    newField->name = getAttributeValue(nodeset, &dataTypeField_Name, attributes,
                                       attributeSize);

    char *value = getAttributeValue(nodeset, &dataTypeField_Value, attributes,
                                    attributeSize);
    if (value) {
        newField->value = atoi(value);
        dataTypeNode->definition->isEnum =
            !dataTypeNode->definition->isOptionSet;
    } else {
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

void
Nodeset_setDisplayName(Nodeset *nodeset, NL_Node *node,
                       size_t attributeSize, const char **attributes) {
    node->displayName.locale =
        UA_STRING(getAttributeValue(nodeset, &attrLocale, attributes, attributeSize));
}

void
Nodeset_DisplayNameFinish(const Nodeset *nodeset, NL_Node *node, char *text) {
    node->displayName.text = UA_STRING(text);
}

void
Nodeset_setDescription(Nodeset *nodeset, NL_Node *node,
                       size_t attributeSize, const char **attributes) {
    node->description.locale =
        UA_STRING(getAttributeValue(nodeset, &attrLocale, attributes, attributeSize));
}

void
Nodeset_DescriptionFinish(const Nodeset *nodeset, NL_Node *node, char *text) {
    node->description.text = UA_STRING(text);
}

void
Nodeset_setInverseName(Nodeset *nodeset, NL_Node *node,
                       size_t attributeSize, const char **attributes) {
    if (node->nodeClass == NODECLASS_REFERENCETYPE) {
        ((NL_ReferenceTypeNode *)node)->inverseName.locale =
            UA_STRING(getAttributeValue(nodeset, &attrLocale, attributes, attributeSize));
    }
}

void
Nodeset_InverseNameFinish(const Nodeset *nodeset, NL_Node *node, char *text) {
    if(node->nodeClass == NODECLASS_REFERENCETYPE)
        ((NL_ReferenceTypeNode *)node)->inverseName.text = UA_STRING(text);
}

void
Nodeset_forEachNode(Nodeset *nodeset, void *context,
                    NodesetLoader_forEachNode_Func fn) {
    NodeContainer *c = &nodeset->sortedNodes;
    for(size_t i = 0; i < c->size; i++) {
        NL_Node *node = c->nodes[i];
        fn(context, node);
    }
}
