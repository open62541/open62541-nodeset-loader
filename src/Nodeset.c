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

static enum ZIP_CMP
compareNodeId(const void *a, const void *b) {
    return (enum ZIP_CMP)
        UA_NodeId_order((const UA_NodeId*)a, (const UA_NodeId*)b);
}

ZIP_FUNCTIONS(NodeTree, NL_Node, treeEntry, UA_NodeId, id, compareNodeId)

static void
NodeList_append(NodeList *list, NL_Node *node) {
    node->sortNext = NULL;
    if(list->tail)
        list->tail->sortNext = node;
    else
        list->head = node;
    list->tail = node;
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
    if(!node)
        return;
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
    } else if(node->nodeClass == NODECLASS_VARIABLETYPE) {
        NL_VariableTypeNode *varTypeNode = (NL_VariableTypeNode*)node;
        UA_NodeId_clear(&varTypeNode->datatype);
    } else if(node->nodeClass == NODECLASS_DATATYPE) {
        NL_DataTypeNode *dtNode = (NL_DataTypeNode*)node;
        if(dtNode->definition) {
            for(size_t i = 0; i < dtNode->definition->fieldCnt; i++)
                UA_NodeId_clear(&dtNode->definition->fields[i].dataType);
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
    if(!list)
        return;
    for(size_t i = 0; i < list->size; i++)
        UA_NodeId_clear(&list->data[i].id);
    free(list->data);
    free(list);
}

static bool
parseNodeId(const Nodeset *nodeset, char *s, UA_NodeId *out) {
    if(!s)
        return false;
    return UA_NodeId_parseEx(out, UA_STRING(s), nodeset->fc->nsMapping) ==
        UA_STATUSCODE_GOOD;
}

static bool
parseQualifiedName(const Nodeset *nodeset, char *s, UA_QualifiedName *out) {
    if(!s)
        return false;
    UA_StatusCode res =
        UA_QualifiedName_parseEx(out, UA_STRING(s), nodeset->fc->nsMapping);
    if(res != UA_STATUSCODE_GOOD)
        return false;
    out->namespaceIndex = UA_NamespaceMapping_remote2Local(
        nodeset->fc->nsMapping, out->namespaceIndex);
    return true;
}

static bool
alias2Id(const Nodeset *nodeset, char *name, UA_NodeId *out) {
    const UA_NodeId *alias = AliasList_getNodeId(nodeset->aliasList, name);
    if(!alias)
        return parseNodeId(nodeset, name, out);
    return UA_NodeId_copy(alias, out) == UA_STATUSCODE_GOOD;
}

Nodeset *
Nodeset_new(UA_Logger *logger) {
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));
    if(!nodeset)
        return NULL;

    nodeset->aliasList = AliasList_new();
    if(!nodeset->aliasList) {
        Nodeset_cleanup(nodeset);
        return NULL;
    }
    ZIP_INIT(&nodeset->nodeTree);
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

static NL_Node *
Nodeset_findByNodeId(Nodeset *nodeset, const UA_NodeId *key) {
    return ZIP_FIND(NodeTree, &nodeset->nodeTree, key);
}

static const UA_NodeId hasTypeDef = {0, UA_NODEIDTYPE_NUMERIC, {40}};

static bool
nodeRefsReady(NL_Node *node) {
    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        if(!ref->targetPtr)
            continue;
        if(ref->targetPtr->isSorted)
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
    NodeList *pending = &nodeset->pending[nodeClass];
    bool changed;

    // Check all nodes if they can be inserted now.
    // Retry until all nodes have been added or a fixpoint was reached.
    do {
        changed = false;
        NL_Node **next = &pending->head;
        NL_Node *previous = NULL;
        while(*next) {
            NL_Node *node = *next;
            if(!nodeRefsReady(node)) {
                previous = node;
                next = &node->sortNext;
                continue;
            }

            *next = node->sortNext;
            if(node == pending->tail)
                pending->tail = previous;
            NodeList_append(&nodeset->sorted, node);
            node->isSorted = true;
            changed = true;
        }
    } while(changed);

    return !pending->head;
}

static void *
resolveReferenceTargets(void *context, NL_Node *node) {
    Nodeset *nodeset = (Nodeset*)context;
    for(NL_Reference *ref = node->refs; ref; ref = ref->next)
        ref->targetPtr = Nodeset_findByNodeId(nodeset, &ref->target);
    return NULL;
}

static void *
resetNodeState(void *context, NL_Node *node) {
    (void)context;
    node->isSorted = false;
    return NULL;
}

bool Nodeset_sort(Nodeset *nodeset) {
    if(!nodeset)
        return false;

    // Keep already-sorted nodes available as dependencies if more files were
    // imported after an earlier call.
    for(NL_Node *node = nodeset->sorted.head; node; node = node->sortNext)
        node->isSorted = true;

    // Insert a pointer to the target node for all references.
    // If the target is not found in the tree, assume it already exists in the server.
    ZIP_ITER(NodeTree, &nodeset->nodeTree, resolveReferenceTargets, nodeset);

    // Add ReferenceTypes
    bool done = Nodeset_sortNodeClass(nodeset, NODECLASS_REFERENCETYPE);
    bool allDone = done;
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add ReferenceType hierarchy");

    // Add DataTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_DATATYPE);
    allDone &= done;
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add DataType hierarchy");

    // Add VariableTypes
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLETYPE);
    allDone &= done;
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add VariableType hierarchy");

    // Add Views
    done = Nodeset_sortNodeClass(nodeset, NODECLASS_VIEW);
    allDone &= done;
    if(!done)
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Cannot add Views");

    // Add ObjectType, Object, Method and Variable
    NL_Node *lastSorted;
 retry:
    lastSorted = nodeset->sorted.tail;
    done = true;
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECTTYPE);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_OBJECT);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_METHOD);
    done &= Nodeset_sortNodeClass(nodeset, NODECLASS_VARIABLE);
    if(done)
        goto finish;
    if(lastSorted == nodeset->sorted.tail) {
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: Infinite loop in the references");
        goto finish;
    }
    goto retry;

 finish:
    // Reset the temporary sorting state.
    ZIP_ITER(NodeTree, &nodeset->nodeTree, resetNodeState, NULL);
    return allDone && done;
}

static void *
deleteNode(void *context, NL_Node *node) {
    (void)context;
    Node_delete(node);
    return NULL;
}

void Nodeset_cleanup(Nodeset *nodeset) {
    if(!nodeset)
        return;
    AliasList_delete(nodeset->aliasList);
    ZIP_ITER(NodeTree, &nodeset->nodeTree, deleteNode, NULL);
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

static bool
extractAttributes(Nodeset *nodeset, NL_Node *node,
                  const XmlAttributes *attributes) {
    if(!parseNodeId(nodeset, getAttributeValue(&attrNodeId, attributes),
                    &node->id) ||
       !parseQualifiedName(nodeset,
                           getAttributeValue(&attrBrowseName, attributes),
                           &node->browseName))
        return false;
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
        if(!alias2Id(nodeset, datatype,
                     &((NL_VariableNode *)node)->datatype))
            return false;
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
        if(!alias2Id(nodeset, datatype,
                     &((NL_VariableTypeNode *)node)->datatype))
            return false;
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
    return true;
}

NL_Node *
Nodeset_newNode(Nodeset *nodeset, NL_NodeClass nodeClass,
                const XmlAttributes *attributes) {
    NL_Node *node = Node_new(nodeClass);
    if(!node)
        return NULL;
    node->nodeClass = nodeClass;
    if(!extractAttributes(nodeset, node, attributes)) {
        Node_delete(node);
        return NULL;
    }

    NodeList *pending = &nodeset->pending[node->nodeClass];
    NodeList_append(pending, node);
    ZIP_INSERT(NodeTree, &nodeset->nodeTree, node);
    return node;
}

NL_Reference *
Nodeset_newReference(Nodeset *nodeset, NL_Node *node,
                     const XmlAttributes *attributes) {
    NL_Reference *newRef = (NL_Reference *)calloc(1, sizeof(NL_Reference));
    if(!newRef)
        return NULL;

    char *isForwardString =
        getAttributeValue(&attrIsForward, attributes);
    if(!strcmp("true", isForwardString)) {
        newRef->isForward = true;
    } else {
        newRef->isForward = false;
    }

    char *aliasIdString =
        getAttributeValue(&attrReferenceType, attributes);
    if(!alias2Id(nodeset, aliasIdString, &newRef->refType)) {
        free(newRef);
        return NULL;
    }

    newRef->next = node->refs;
    node->refs = newRef;
    return newRef;
}

bool
Nodeset_newReference_finish(Nodeset *nodeset, NL_Reference *ref,
                            char *idString) {
    return alias2Id(nodeset, idString, &ref->target);
}

static NL_DataTypeDefinitionField *
DataTypeNode_addDefinitionField(NL_DataTypeDefinition *def) {
    if(def->fieldCnt >= SIZE_MAX / sizeof(NL_DataTypeDefinitionField))
        return NULL;
    size_t newCount = def->fieldCnt + 1;
    NL_DataTypeDefinitionField *fields = (NL_DataTypeDefinitionField *)
        realloc(def->fields, newCount * sizeof(NL_DataTypeDefinitionField));
    if(!fields)
        return NULL;
    def->fields = fields;
    def->fieldCnt = newCount;
    return &fields[newCount - 1];
}

bool Nodeset_addDataTypeDefinition(NL_Node *node,
                                   const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if(dataTypeNode->definition)
        return false;
    dataTypeNode->definition = (NL_DataTypeDefinition *)
        calloc(1, sizeof(NL_DataTypeDefinition));
    if(!dataTypeNode->definition)
        return false;
    dataTypeNode->definition->isUnion =
        !strcmp("true", getAttributeValue(&dataTypeDefinition_IsUnion,
                                          attributes));
    dataTypeNode->definition->isOptionSet =
        !strcmp("true", getAttributeValue(&dataTypeDefinition_IsOptionSet,
                                          attributes));
    return true;
}

bool Nodeset_addDataTypeField(Nodeset *nodeset, NL_Node *node,
                              const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if(!dataTypeNode->definition)
        return false;

    NL_DataTypeDefinitionField *newField =
        DataTypeNode_addDefinitionField(dataTypeNode->definition);
    if(!newField)
        return false;
    memset(newField, 0, sizeof(NL_DataTypeDefinitionField));

    newField->name = getAttributeValue(&dataTypeField_Name, attributes);
    if(!newField->name) {
        dataTypeNode->definition->fieldCnt--;
        return false;
    }

    char *value = getAttributeValue(&dataTypeField_Value, attributes);
    if (value) {
        newField->value = atoi(value);
        dataTypeNode->definition->isEnum =
            !dataTypeNode->definition->isOptionSet;
    } else {
        if(!alias2Id(nodeset,
                     getAttributeValue(&dataTypeField_DataType, attributes),
                     &newField->dataType)) {
            dataTypeNode->definition->fieldCnt--;
            return false;
        }
        newField->valueRank = atoi(getAttributeValue(&attrValueRank,
                                                     attributes));
        char *isOptional =
            getAttributeValue(&dataTypeField_IsOptional, attributes);
        newField->isOptional = !strcmp("true", isOptional);
    }
    return true;
}

Alias *
Nodeset_newAlias(Nodeset *nodeset, const XmlAttributes *attributes) {
    return AliasList_newAlias(nodeset->aliasList,
                              getAttributeValue(&attrAlias, attributes));
}

bool
Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString) {
    return parseNodeId(nodeset, idString, &alias->id);
}

bool
Nodeset_newNamespaceFinish(Nodeset *nodeset, char *namespaceUri) {
    if(!namespaceUri)
        return false;
    UA_String uri = UA_STRING(namespaceUri);
    return nodeset->fc->addNamespace(nodeset->fc->userContext,
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
