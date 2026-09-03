/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "Nodeset.h"
#include <open62541/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATTRIBUTE_NODEID "NodeId"
#define ATTRIBUTE_BROWSENAME "BrowseName"
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

static const NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL};
static const NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL};
static const NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, "0"};
static const NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24"};
static const NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1"};
static const NodeAttribute attrMinimumSamplingInterval = {
    ATTRIBUTE_MINIMUMSAMPLINGINTERVAL, "-1"};
static const NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, ""};
static const NodeAttribute attrIsAbstract = {ATTRIBUTE_ISABSTRACT, "false"};
static const NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true"};
static const NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL};
static const NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL};
static const NodeAttribute attrExecutable = {"Executable", "true"};
static const NodeAttribute attrUserExecutable = {"UserExecutable", "true"};
static const NodeAttribute attrAccessLevel = {"AccessLevel", "1"};
static const NodeAttribute attrUserAccessLevel = {"UserAccessLevel", "1"};
static const NodeAttribute attrSymmetric = {ATTRIBUTE_SYMMETRIC, "false"};
static const NodeAttribute dataTypeDefinition_IsUnion = {"IsUnion", "false"};
static const NodeAttribute dataTypeDefinition_IsOptionSet = {"IsOptionSet", "false"};
static const NodeAttribute dataTypeField_Name = {"Name", NULL};
static const NodeAttribute dataTypeField_DataType = {"DataType", "i=24"};
static const NodeAttribute dataTypeField_Value = {"Value", NULL};
static const NodeAttribute dataTypeField_IsOptional = {"IsOptional", "false"};
static const NodeAttribute attrLocale = {"Locale", NULL};
static const NodeAttribute attrHistorizing = {ATTRIBUTE_HISTORIZING, "false"};
static const NodeAttribute attrContainsNoLoops = {
    ATTRIBUTE_CONTAINSNOLOOPS, "false"};

#define MAX_ALIAS 300

struct Alias {
    char *name;
    UA_NodeId id;
};

struct AliasList {
    Alias *data;
    size_t size;
};

struct NodesetTextBuffer {
    char *data;
    NodesetTextBuffer *next;
};

static bool
NodeContainer_init(NodeContainer *container, size_t initialSize) {
    memset(container, 0, sizeof(NodeContainer));
    container->nodes = (NL_Node **)calloc(initialSize, sizeof(NL_Node*));
    if(!container->nodes)
        return false;
    container->capacity = initialSize;
    return true;
}

static bool
NodeContainer_add(NodeContainer *container, NL_Node *node) {
    if(container->size == container->capacity) {
        NL_Node **nodes = (NL_Node **)realloc(
            container->nodes, container->size * 2 * sizeof(NL_Node*));
        if(!nodes)
            return false;
        container->nodes = nodes;
        container->capacity *= 2;
    }
    container->nodes[container->size++] = node;
    return true;
}

static void
NodeContainer_remove(NodeContainer *container, size_t index) {
    container->nodes[index] = container->nodes[container->size - 1];
    container->size--;
}

static void
NodeContainer_clear(NodeContainer *container) {
    free(container->nodes);
    memset(container, 0, sizeof(NodeContainer));
}

static NL_Node *
Node_new(NL_NodeClass nodeClass) {
    size_t nodeSize = 0;
    switch(nodeClass) {
    case NODECLASS_VARIABLE:
        nodeSize = sizeof(NL_VariableNode);
        break;
    case NODECLASS_OBJECT:
        nodeSize = sizeof(NL_ObjectNode);
        break;
    case NODECLASS_OBJECTTYPE:
        nodeSize = sizeof(NL_ObjectTypeNode);
        break;
    case NODECLASS_REFERENCETYPE:
        nodeSize = sizeof(NL_ReferenceTypeNode);
        break;
    case NODECLASS_VARIABLETYPE:
        nodeSize = sizeof(NL_VariableTypeNode);
        break;
    case NODECLASS_DATATYPE:
        nodeSize = sizeof(NL_DataTypeNode);
        break;
    case NODECLASS_METHOD:
        nodeSize = sizeof(NL_MethodNode);
        break;
    case NODECLASS_VIEW:
        nodeSize = sizeof(NL_ViewNode);
        break;
    }
    if(nodeSize == 0)
        return NULL;
    return (NL_Node*)calloc(1, nodeSize);
}

static void
Node_delete(NL_Node *node) {
    UA_NodeId_clear(&node->id);
    UA_QualifiedName_clear(&node->browseName);

    NL_Reference *ref = node->refs;
    while(ref) {
        NL_Reference *next = ref->next;
        UA_NodeId_clear(&ref->target);
        UA_NodeId_clear(&ref->refType);
        free(ref);
        ref = next;
    }

    if(node->nodeClass == NODECLASS_VARIABLE) {
        NL_VariableNode *varNode = (NL_VariableNode*)node;
        UA_String_clear(&varNode->value);
        UA_NodeId_clear(&varNode->datatype);
    } else if(node->nodeClass == NODECLASS_DATATYPE) {
        NL_DataTypeNode *dtNode = (NL_DataTypeNode*)node;
        if(dtNode->definition) {
            free(dtNode->definition->fields);
            free(dtNode->definition);
        }
    }
    free(node);
}

static AliasList *
AliasList_new(void) {
    AliasList *list = (AliasList*)calloc(1, sizeof(AliasList));
    if(!list)
        return NULL;
    list->data = (Alias*)calloc(MAX_ALIAS, sizeof(Alias));
    if(!list->data) {
        free(list);
        return NULL;
    }
    return list;
}

static Alias *
AliasList_newAlias(AliasList *list, char *name) {
    if(list->size >= MAX_ALIAS)
        return NULL;
    Alias *alias = &list->data[list->size++];
    alias->name = name;
    return alias;
}

static const UA_NodeId *
AliasList_getNodeId(const AliasList *list, const char *name) {
    if(!name)
        return NULL;
    for(Alias *alias = list->data; alias != list->data + list->size; alias++) {
        if(!strcmp(name, alias->name))
            return &alias->id;
    }
    return NULL;
}

static void
AliasList_delete(AliasList *list) {
    free(list->data);
    free(list);
}

static UA_NodeId
parseNodeId(const Nodeset *nodeset, char *s) {
    UA_NodeId n;
    UA_NodeId_parseEx(&n, UA_STRING(s), nodeset->fc->nsMapping);
    return n;
}

static UA_QualifiedName
parseQualifiedName(const Nodeset *nodeset, char *s) {
    UA_QualifiedName qn;
    UA_QualifiedName_parseEx(&qn, UA_STRING(s), nodeset->fc->nsMapping);
    qn.namespaceIndex = UA_NamespaceMapping_remote2Local(nodeset->fc->nsMapping, qn.namespaceIndex);
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
Nodeset_new(UA_Logger *logger) {
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));
    if(!nodeset)
        return NULL;

    nodeset->aliasList = AliasList_new();
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

bool
Nodeset_ownTextBuffer(Nodeset *nodeset, char *data) {
    NodesetTextBuffer *buffer =
        (NodesetTextBuffer*)malloc(sizeof(NodesetTextBuffer));
    if(!buffer)
        return false;
    buffer->data = data;
    buffer->next = nodeset->textBuffers;
    nodeset->textBuffers = buffer;
    return true;
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

static const UA_NodeId hasTypeDef = {0, UA_NODEIDTYPE_NUMERIC, {40}};

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
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add ReferenceType hierarchy");

    // Add DataTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_DATATYPE);
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add DataType hierarchy");

    // Add VariableTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLETYPE);
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add VariableType hierarchy");

    // Add Views
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VIEW);
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add Views");

    // Add ObjectType, Object, Method and Variable
    size_t totalSorted;
 retry:
    totalSorted = nodeset->sortedNodes.size;
    done = true;
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECTTYPE);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECT);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_METHOD);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLE);
    if(done)
        goto finish;
    if(totalSorted == nodeset->sortedNodes.size) {
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Infinite loop in the references");
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
    AliasList_delete(nodeset->aliasList);
    for (size_t cnt = 0; cnt < NL_NODECLASS_COUNT; cnt++) {
        NodeContainer_clear(&nodeset->nodes[cnt]);
    }
    for(size_t i = 0; i < nodeset->allNodes.size; i++) {
        Node_delete(nodeset->allNodes.nodes[i]);
    }
    NodeContainer_clear(&nodeset->allNodes);
    NodeContainer_clear(&nodeset->sortedNodes);
    NodesetTextBuffer *buffer = nodeset->textBuffers;
    while(buffer) {
        NodesetTextBuffer *next = buffer->next;
        free(buffer->data);
        free(buffer);
        buffer = next;
    }
    free(nodeset);
}

static char *
getAttributeValue(const NodeAttribute *attr,
                  const XmlAttributes *attributes) {
    for(size_t i = 0; i < attributes->size; i++) {
        const XmlToken *token = &attributes->tokens[i];
        const char *name = attributes->text + token->name;
        const char *colon = strrchr(name, ':');
        if(colon)
            name = colon + 1;
        if(strcmp(name, attr->name))
            continue;
        return attributes->text + token->content;
    }

    // we return the defaultValue, if NULL or not, following code has to cope
    // with it
    return attr->defaultValue;
}

static void
extractAttributes(Nodeset *nodeset, NL_Node *node,
                  const XmlAttributes *attributes) {
    node->id =
        parseNodeId(nodeset, getAttributeValue(&attrNodeId, attributes));
    node->browseName =
        parseQualifiedName(nodeset,
                           getAttributeValue(&attrBrowseName, attributes));
    switch (node->nodeClass) {
    case NODECLASS_OBJECTTYPE:
        ((NL_ObjectTypeNode *)node)->isAbstract =
            getAttributeValue(&attrIsAbstract, attributes);
        break;

    case NODECLASS_OBJECT:
        ((NL_ObjectNode *)node)->eventNotifier =
            getAttributeValue(&attrEventNotifier, attributes);
        break;

    case NODECLASS_VARIABLE: {
        char *datatype = getAttributeValue(&attrDataType, attributes);
        ((NL_VariableNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableNode *)node)->valueRank =
            getAttributeValue(&attrValueRank, attributes);
        ((NL_VariableNode *)node)->minimumSamplingInterval =
            getAttributeValue(&attrMinimumSamplingInterval, attributes);
        ((NL_VariableNode *)node)->arrayDimensions =
            getAttributeValue(&attrArrayDimensions, attributes);
        ((NL_VariableNode *)node)->accessLevel =
            getAttributeValue(&attrAccessLevel, attributes);
        ((NL_VariableNode *)node)->userAccessLevel =
            getAttributeValue(&attrUserAccessLevel, attributes);
        ((NL_VariableNode *)node)->historizing =
            getAttributeValue(&attrHistorizing, attributes);
        break;
    }

    case NODECLASS_VARIABLETYPE: {
        ((NL_VariableTypeNode *)node)->valueRank =
            getAttributeValue(&attrValueRank, attributes);
        char *datatype = getAttributeValue(&attrDataType, attributes);
        ((NL_VariableTypeNode *)node)->datatype = alias2Id(nodeset, datatype);
        ((NL_VariableTypeNode *)node)->arrayDimensions =
            getAttributeValue(&attrArrayDimensions, attributes);
        ((NL_VariableTypeNode *)node)->isAbstract =
            getAttributeValue(&attrIsAbstract, attributes);
        break;
    }

    case NODECLASS_DATATYPE:
        ((NL_DataTypeNode *)node)->isAbstract =
            getAttributeValue(&attrIsAbstract, attributes);
        break;

    case NODECLASS_METHOD:
        ((NL_MethodNode *)node)->executable =
            getAttributeValue(&attrExecutable, attributes);
        ((NL_MethodNode *)node)->userExecutable =
            getAttributeValue(&attrUserExecutable, attributes);
        break;

    case NODECLASS_REFERENCETYPE:
        ((NL_ReferenceTypeNode *)node)->symmetric =
            getAttributeValue(&attrSymmetric, attributes);
        break;

    case NODECLASS_VIEW:
        ((NL_ViewNode *)node)->containsNoLoops =
            getAttributeValue(&attrContainsNoLoops, attributes);
        ((NL_ViewNode *)node)->eventNotifier =
            getAttributeValue(&attrEventNotifier, attributes);
        break;

    default:
        break;
    }
}

NL_Node *
Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                const XmlAttributes *attributes) {
    NL_Node *node = Node_new(nodeClass);
    node->nodeClass = nodeClass;
    extractAttributes(nodeset, node, attributes);
    NodeContainer_add(&nodeset->nodes[node->nodeClass], node);
    NodeContainer_add(&nodeset->allNodes, node);
    return node;
}

NL_Reference *
Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                     const XmlAttributes *attributes) {
    NL_Reference *newRef = (NL_Reference *)calloc(1, sizeof(NL_Reference));

    char *isForwardString =
        getAttributeValue(&attrIsForward, attributes);
    if(!strcmp("true", isForwardString)) {
        newRef->isForward = true;
    } else {
        newRef->isForward = false;
    }

    char *aliasIdString =
        getAttributeValue(&attrReferenceType, attributes);
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

static NL_DataTypeDefinitionField *
DataTypeNode_addDefinitionField(NL_DataTypeDefinition *def) {
    def->fieldCnt++;
    def->fields = (NL_DataTypeDefinitionField *)
        realloc(def->fields, def->fieldCnt * sizeof(NL_DataTypeDefinitionField));
    if(!def->fields)
        return NULL;
    return &def->fields[def->fieldCnt - 1];
}

void Nodeset_addDataTypeDefinition(NL_Node *node,
                                   const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    dataTypeNode->definition = (NL_DataTypeDefinition *)
        calloc(1, sizeof(NL_DataTypeDefinition));
    dataTypeNode->definition->isUnion =
        !strcmp("true", getAttributeValue(&dataTypeDefinition_IsUnion,
                                          attributes));
    dataTypeNode->definition->isOptionSet =
        !strcmp("true", getAttributeValue(&dataTypeDefinition_IsOptionSet,
                                          attributes));
}

void Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;

    NL_DataTypeDefinitionField *newField =
        DataTypeNode_addDefinitionField(dataTypeNode->definition);
    memset(newField, 0, sizeof(NL_DataTypeDefinitionField));

    newField->name = getAttributeValue(&dataTypeField_Name, attributes);

    char *value = getAttributeValue(&dataTypeField_Value, attributes);
    if (value) {
        newField->value = atoi(value);
        dataTypeNode->definition->isEnum =
            !dataTypeNode->definition->isOptionSet;
    } else {
        newField->dataType = alias2Id(
            nodeset, getAttributeValue(&dataTypeField_DataType, attributes));
        newField->valueRank = atoi(getAttributeValue(&attrValueRank,
                                                     attributes));
        char *isOptional =
            getAttributeValue(&dataTypeField_IsOptional, attributes);
        newField->isOptional = !strcmp("true", isOptional);
    }
}

Alias *
Nodeset_newAlias(Nodeset *nodeset, const XmlAttributes *attributes) {
    return AliasList_newAlias(nodeset->aliasList,
                              getAttributeValue(&attrAlias, attributes));
}

void
Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString) {
    alias->id = parseNodeId(nodeset, idString);
}

void
Nodeset_newNamespaceFinish(Nodeset *nodeset, char *namespaceUri) {
    UA_String uri = UA_STRING(namespaceUri);
    nodeset->fc->addNamespace(nodeset->fc->userContext,
                              1, &uri, nodeset->fc->nsMapping);
}

void
Nodeset_setDisplayName(NL_Node *node, const XmlAttributes *attributes) {
    node->displayName.locale =
        UA_STRING(getAttributeValue(&attrLocale, attributes));
}

void
Nodeset_DisplayNameFinish(NL_Node *node, char *text) {
    node->displayName.text = UA_STRING(text);
}

void
Nodeset_setDescription(NL_Node *node, const XmlAttributes *attributes) {
    node->description.locale =
        UA_STRING(getAttributeValue(&attrLocale, attributes));
}

void
Nodeset_DescriptionFinish(NL_Node *node, char *text) {
    node->description.text = UA_STRING(text);
}

void
Nodeset_setInverseName(NL_Node *node, const XmlAttributes *attributes) {
    if (node->nodeClass == NODECLASS_REFERENCETYPE) {
        ((NL_ReferenceTypeNode *)node)->inverseName.locale =
            UA_STRING(getAttributeValue(&attrLocale, attributes));
    }
}

void
Nodeset_InverseNameFinish(NL_Node *node, char *text) {
    if(node->nodeClass == NODECLASS_REFERENCETYPE)
        ((NL_ReferenceTypeNode *)node)->inverseName.text = UA_STRING(text);
}
