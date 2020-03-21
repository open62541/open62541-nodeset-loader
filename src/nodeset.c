/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"
#include "sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Nodeset_addNodeToSort(TNode *node);
static TNodeId alias2Id(Nodeset *nodeset, const char *alias);
static bool isHierachicalReference(Nodeset *nodeset, const Reference *ref);

#define MAX_OBJECTTYPES 1000
#define MAX_OBJECTS 100000
#define MAX_METHODS 1000
#define MAX_DATATYPES 1000
#define MAX_VARIABLES 1000000
#define MAX_REFERENCETYPES 1000
#define MAX_VARIABLETYPES 1000
#define MAX_HIERACHICAL_REFS 50
#define MAX_ALIAS 100
#define MAX_REFCOUNTEDCHARS 10000000
#define MAX_REFCOUNTEDREFS 1000000

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

typedef struct {
    const char *name;
    char *defaultValue;
    bool optional;
} NodeAttribute;

const NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL, false};
const NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL, false};
const NodeAttribute attrParentNodeId = {ATTRIBUTE_PARENTNODEID, NULL, true};
const NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, NULL, true};
const NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24", false};
const NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1", false};
const NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, "", false};
const NodeAttribute attrIsAbstract = {ATTRIBUTE_ISABSTRACT, "false", false};
const NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true", false};
const NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL, true};
const NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL, false};
const NodeAttribute attrExecutable = {"Executable", "true", false};

const char *hierachicalReferences[MAX_HIERACHICAL_REFS] = {
    "Organizes",  "HasEventSource", "HasNotifier", "Aggregates",
    "HasSubtype", "HasComponent",   "HasProperty"};

TNodeId translateNodeId(const TNamespace *namespaces, TNodeId id) {
    if(id.nsIdx > 0) {
        id.nsIdx = (int)namespaces[id.nsIdx].idx;
        return id;
    }
    return id;
}

TBrowseName translateBrowseName(const TNamespace *namespaces, TBrowseName bn)
{
    if (bn.nsIdx > 0)
    {
        bn.nsIdx = (uint16_t)namespaces[bn.nsIdx].idx;
        return bn;
    }
    return bn;
}

TNodeId extractNodedId(const TNamespace *namespaces, char *s) {
    if(s == NULL) {
        TNodeId id;
        id.id = 0;
        id.nsIdx = 0;
        id.idString = "null";
        return id;
    }
    TNodeId id;
    id.nsIdx = 0;
    id.idString = s;
    char *idxSemi = strchr(s, ';');
    if(idxSemi == NULL) {
        id.id = s;
        return id;
    } else {
        id.nsIdx = atoi(&s[3]);
        id.id = idxSemi + 1;
    }
    return translateNodeId(namespaces, id);
}

TBrowseName extractBrowseName(const TNamespace* namespaces, char*s)
{
    TBrowseName bn;
    bn.nsIdx = 0;
    char* bnName = strchr(s, ':');
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

static TNodeId alias2Id(Nodeset *nodeset, const char *alias) {
    for(size_t cnt = 0; cnt < nodeset->aliasSize; cnt++) {
        if(!strcmp(alias, nodeset->aliasArray[cnt]->name)) {
            return nodeset->aliasArray[cnt]->id;
        }
    }
    TNodeId id;
    id.id = 0;
    return id;
}

Nodeset *Nodeset_new(addNamespaceCb nsCallback) {
    Nodeset *nodeset = (Nodeset *)calloc(1, sizeof(Nodeset));
    nodeset->aliasArray = (Alias **)malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->countedRefs = (Reference **)malloc(sizeof(Reference *) * MAX_REFCOUNTEDREFS);
    nodeset->refsSize = 0;
    nodeset->charArena = CharArenaAllocator_new(1024 * 1024 * 5);
    // objects
    nodeset->nodes[NODECLASS_OBJECT] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECT]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_OBJECTS);
    nodeset->nodes[NODECLASS_OBJECT]->cnt = 0;
    // variables
    nodeset->nodes[NODECLASS_VARIABLE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_VARIABLES);
    nodeset->nodes[NODECLASS_VARIABLE]->cnt = 0;
    // methods
    nodeset->nodes[NODECLASS_METHOD] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_METHOD]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_METHODS);
    nodeset->nodes[NODECLASS_METHOD]->cnt = 0;
    // objecttypes
    nodeset->nodes[NODECLASS_OBJECTTYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt = 0;
    // datatypes
    nodeset->nodes[NODECLASS_DATATYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_DATATYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_DATATYPE]->cnt = 0;
    // referencetypes
    nodeset->nodes[NODECLASS_REFERENCETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt = 0;
    // variabletypes
    nodeset->nodes[NODECLASS_VARIABLETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_VARIABLETYPES);
    nodeset->nodes[NODECLASS_VARIABLETYPE]->cnt = 0;
    // known hierachical refs
    nodeset->hierachicalRefs = hierachicalReferences;
    nodeset->hierachicalRefsSize = 7;

    TNamespaceTable *table = (TNamespaceTable *)malloc(sizeof(TNamespaceTable));
    table->cb = nsCallback;
    table->size = 1;
    table->ns = (TNamespace *)malloc((sizeof(TNamespace)));
    table->ns[0].idx = 0;
    table->ns[0].name = "http://opcfoundation.org/UA/";
    nodeset->namespaceTable = table;
    // init sorting
    init();
    return nodeset;
}

static void Nodeset_addNode(Nodeset *nodeset, TNode *node) {
    size_t cnt = nodeset->nodes[node->nodeClass]->cnt;
    nodeset->nodes[node->nodeClass]->nodes[cnt] = node;
    nodeset->nodes[node->nodeClass]->cnt++;
}

static void Nodeset_addNodeToSort(TNode *node) { addNodeToSort(node); }

bool Nodeset_getSortedNodes(Nodeset *nodeset, void *userContext, addNodeCb callback,
                            ValueInterface *valIf) {

#ifdef XMLIMPORT_TRACE
    printf("--- namespace table ---\n");
    printf("FileIdx ServerIdx URI\n");
    for(size_t fileIndex = 0; fileIndex < nodeset->namespaceTable->size; fileIndex++) {
        printf("%zu\t%zu\t%s\n", fileIndex, nodeset->namespaceTable->ns[fileIndex].idx,
               nodeset->namespaceTable->ns[fileIndex].name);
    }
#endif

    if(!sort(nodeset, Nodeset_addNode)) {
        return false;
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_OBJECT]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_OBJECT]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_METHOD]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_METHOD]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_DATATYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_DATATYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_VARIABLETYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_VARIABLE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_VARIABLE]->nodes[cnt]);

        valIf->deleteValue(
            &((TVariableNode *)nodeset->nodes[NODECLASS_VARIABLE]->nodes[cnt])->value);
    }
    return true;
}

void Nodeset_cleanup(Nodeset *nodeset) {
    Nodeset *n = nodeset;

    CharArenaAllocator_delete(nodeset->charArena);

    // free refs
    for(size_t cnt = 0; cnt < n->refsSize; cnt++) {
        free(n->countedRefs[cnt]);
    }
    free(n->countedRefs);

    // free alias
    for(size_t cnt = 0; cnt < n->aliasSize; cnt++) {
        free(n->aliasArray[cnt]);
    }
    free(n->aliasArray);

    for(size_t cnt = 0; cnt < NODECLASS_COUNT; cnt++) {
        size_t storedNodes = n->nodes[cnt]->cnt;
        for(size_t nodeCnt = 0; nodeCnt < storedNodes; nodeCnt++) {
            free(n->nodes[cnt]->nodes[nodeCnt]);
        }
        free((void *)n->nodes[cnt]->nodes);
        free((void *)n->nodes[cnt]);
    }

    free(n->namespaceTable->ns);
    free(n->namespaceTable);
    free(n);
    cleanup();
}

static bool isHierachicalReference(Nodeset *nodeset, const Reference *ref) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(!strcmp(ref->refType.idString, nodeset->hierachicalRefs[i])) {
            return true;
        }
    }
    return false;
}

static char *getAttributeValue(Nodeset *nodeset, const NodeAttribute *attr,
                               const char **attributes, int nb_attributes) {
    const int fields = 5;
    for(int i = 0; i < nb_attributes; i++) {
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
    if(attr->defaultValue != NULL || attr->optional) {
        return attr->defaultValue;
    }
    return NULL;
}

static void extractAttributes(Nodeset *nodeset, const TNamespace *namespaces, TNode *node,
                              int attributeSize, const char **attributes) {
    node->id = extractNodedId(
        namespaces, getAttributeValue(nodeset, &attrNodeId, attributes, attributeSize));
    node->browseName =
        extractBrowseName(namespaces, getAttributeValue(nodeset, &attrBrowseName, attributes, attributeSize));
    switch(node->nodeClass) {
        case NODECLASS_OBJECTTYPE: {
            ((TObjectTypeNode *)node)->isAbstract =
                getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize);
            break;
        }
        case NODECLASS_OBJECT: {
            ((TObjectNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                                             attributes, attributeSize));
            ((TObjectNode *)node)->eventNotifier =
                getAttributeValue(nodeset, &attrEventNotifier, attributes, attributeSize);
            break;
        }
        case NODECLASS_VARIABLE: {

            ((TVariableNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                                             attributes, attributeSize));
            char *datatype =
                getAttributeValue(nodeset, &attrDataType, attributes, attributeSize);
            TNodeId aliasId = alias2Id(nodeset, datatype);
            if(aliasId.id != 0) {
                ((TVariableNode *)node)->datatype = aliasId;
            } else {
                ((TVariableNode *)node)->datatype = extractNodedId(namespaces, datatype);
            }
            ((TVariableNode *)node)->valueRank =
                getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize);
            ((TVariableNode *)node)->arrayDimensions = getAttributeValue(
                nodeset, &attrArrayDimensions, attributes, attributeSize);

            break;
        }
        case NODECLASS_VARIABLETYPE: {

            ((TVariableTypeNode *)node)->valueRank =
                getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize);
            char *datatype =
                getAttributeValue(nodeset, &attrDataType, attributes, attributeSize);
            TNodeId aliasId = alias2Id(nodeset, datatype);
            if(aliasId.id != 0) {
                ((TVariableTypeNode *)node)->datatype = aliasId;
            } else {
                ((TVariableTypeNode *)node)->datatype =
                    extractNodedId(namespaces, datatype);
            }
            ((TVariableTypeNode *)node)->arrayDimensions = getAttributeValue(
                nodeset, &attrArrayDimensions, attributes, attributeSize);
            ((TVariableTypeNode *)node)->isAbstract =
                getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize);
            break;
        }
        case NODECLASS_DATATYPE:;
            break;
        case NODECLASS_METHOD:
            ((TMethodNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(nodeset, &attrParentNodeId,
                                                             attributes, attributeSize));
            break;
        case NODECLASS_REFERENCETYPE:;
            break;
        default:;
    }
}

static void initNode(Nodeset *nodeset, TNamespace *namespaces, TNodeClass nodeClass,
                     TNode *node, int nb_attributes, const char **attributes) {
    node->nodeClass = nodeClass;
    node->hierachicalRefs = NULL;
    node->nonHierachicalRefs = NULL;
    node->displayName = NULL;
    node->description = NULL;
    node->writeMask = NULL;
    extractAttributes(nodeset, namespaces, node, nb_attributes, attributes);
}

TNode *Nodeset_newNode(Nodeset *nodeset, TNodeClass nodeClass, int nb_attributes,
                       const char **attributes) {
    TNode *node = NULL;
    switch(nodeClass) {
        case NODECLASS_VARIABLE:
            node = (TNode *)calloc(1, sizeof(TVariableNode));
            break;
        case NODECLASS_OBJECT:
            node = (TNode *)calloc(1, sizeof(TObjectNode));
            break;
        case NODECLASS_OBJECTTYPE:
            node = (TNode *)calloc(1, sizeof(TObjectTypeNode));
            break;
        case NODECLASS_REFERENCETYPE:
            node = (TNode *)calloc(1, sizeof(TReferenceTypeNode));
            break;
        case NODECLASS_VARIABLETYPE:
            node = (TNode *)calloc(1, sizeof(TVariableTypeNode));
            break;
        case NODECLASS_DATATYPE:
            node = (TNode *)calloc(1, sizeof(TDataTypeNode));
            break;
        case NODECLASS_METHOD:
            node = (TNode *)calloc(1, sizeof(TMethodNode));
            break;
    }
    initNode(nodeset, nodeset->namespaceTable->ns, nodeClass, node, nb_attributes,
             attributes);
    return node;
}

Reference *Nodeset_newReference(Nodeset *nodeset, TNode *node, int attributeSize,
                                const char **attributes) {
    Reference *newRef = (Reference *)malloc(sizeof(Reference));
    newRef->target.idString = NULL;
    newRef->target.id = NULL;
    newRef->refType.idString = NULL;
    newRef->refType.id = NULL;
    nodeset->countedRefs[nodeset->refsSize++] = newRef;
    newRef->next = NULL;
    if(!strcmp("true",
               getAttributeValue(nodeset, &attrIsForward, attributes, attributeSize))) {
        newRef->isForward = true;
    } else {
        newRef->isForward = false;
    }
    newRef->refType = extractNodedId(
        nodeset->namespaceTable->ns,
        getAttributeValue(nodeset, &attrReferenceType, attributes, attributeSize));
    if(isHierachicalReference(nodeset, newRef)) {
        Reference *lastRef = node->hierachicalRefs;
        node->hierachicalRefs = newRef;
        newRef->next = lastRef;

    } else {
        Reference *lastRef = node->nonHierachicalRefs;
        node->nonHierachicalRefs = newRef;
        newRef->next = lastRef;
    }
    return newRef;
}

Alias *Nodeset_newAlias(Nodeset *nodeset, int attributeSize, const char **attributes) {
    nodeset->aliasArray[nodeset->aliasSize] = (Alias *)malloc(sizeof(Alias));
    nodeset->aliasArray[nodeset->aliasSize]->id.idString = NULL;
    nodeset->aliasArray[nodeset->aliasSize]->name =
        getAttributeValue(nodeset, &attrAlias, attributes, attributeSize);
    return nodeset->aliasArray[nodeset->aliasSize];
}

void Nodeset_newAliasFinish(Nodeset *nodeset, Alias *alias, char *idString) {
    alias->id = extractNodedId(nodeset->namespaceTable->ns, idString);
    nodeset->aliasSize++;
}

TNamespace *Nodeset_newNamespace(Nodeset *nodeset) {
    nodeset->namespaceTable->size++;
    TNamespace *ns =
        (TNamespace *)realloc(nodeset->namespaceTable->ns,
                              sizeof(TNamespace) * (nodeset->namespaceTable->size));
    nodeset->namespaceTable->ns = ns;
    ns[nodeset->namespaceTable->size - 1].name = NULL;
    return &ns[nodeset->namespaceTable->size - 1];
}

void Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext, char *namespaceUri) {
    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name = namespaceUri;
    int globalIdx = nodeset->namespaceTable->cb(
        userContext, nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name);

    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].idx =
        (size_t)globalIdx;
}

void Nodeset_newNodeFinish(Nodeset *nodeset, TNode *node) {
    Nodeset_addNodeToSort(node);
    if(node->nodeClass == NODECLASS_REFERENCETYPE) {
        Reference *ref = node->hierachicalRefs;
        while(ref) {
            if(!ref->isForward) {
                nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] =
                    node->id.idString;
                break;
            }
            ref = ref->next;
        }
    }
}

void Nodeset_newReferenceFinish(Nodeset *nodeset, Reference *ref, TNode *node,
                                char *targetId) {
    ref->target.idString = targetId;
    ref->target = extractNodedId(nodeset->namespaceTable->ns, ref->target.idString);
}
