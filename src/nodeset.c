/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"
#include "sort.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define free_const(x) free((void *)(long)(x))

Nodeset *nodeset = NULL;

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

NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL, false};
NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL, false};
NodeAttribute attrParentNodeId = {ATTRIBUTE_PARENTNODEID, NULL, true};
NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, NULL, true};
NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24", false};
NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1", false};
NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, "", false};
NodeAttribute attrIsAbstract = {ATTRIBUTE_ARRAYDIMENSIONS, "false", false};
NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true", false};
NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL, true};
NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL, false};

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

TNodeId alias2Id(const char *alias) {
    for(size_t cnt = 0; cnt < nodeset->aliasSize; cnt++) {
        if(strEqual(alias, nodeset->aliasArray[cnt]->name)) {
            return nodeset->aliasArray[cnt]->id;
        }
    }
    TNodeId id;
    id.id = 0;
    return id;
}

void Nodeset_new(addNamespaceCb nsCallback) {
    nodeset = (Nodeset *)malloc(sizeof(Nodeset));
    nodeset->aliasArray = (Alias **)malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->countedRefs =
        (const Reference **)malloc(sizeof(Reference *) * MAX_REFCOUNTEDREFS);
    nodeset->refsSize = 0;
    nodeset->countedChars = (const char **)malloc(sizeof(char *) * MAX_REFCOUNTEDCHARS);
    nodeset->charsSize = 0;
    // objects
    nodeset->nodes[NODECLASS_OBJECT] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECT]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_OBJECTS);
    nodeset->nodes[NODECLASS_OBJECT]->cnt = 0;
    // variables
    nodeset->nodes[NODECLASS_VARIABLE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLE]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_VARIABLES);
    nodeset->nodes[NODECLASS_VARIABLE]->cnt = 0;
    // methods
    nodeset->nodes[NODECLASS_METHOD] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_METHOD]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_METHODS);
    nodeset->nodes[NODECLASS_METHOD]->cnt = 0;
    // objecttypes
    nodeset->nodes[NODECLASS_OBJECTTYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt = 0;
    // datatypes
    nodeset->nodes[NODECLASS_DATATYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_DATATYPE]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_DATATYPE]->cnt = 0;
    // referencetypes
    nodeset->nodes[NODECLASS_REFERENCETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt = 0;
    // variabletypes
    nodeset->nodes[NODECLASS_VARIABLETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes =
        (const TNode **)malloc(sizeof(TNode *) * MAX_VARIABLETYPES);
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
}

static void Nodeset_addNode(const TNode *node) {
    size_t cnt = nodeset->nodes[node->nodeClass]->cnt;
    nodeset->nodes[node->nodeClass]->nodes[cnt] = node;
    nodeset->nodes[node->nodeClass]->cnt++;
}

void Nodeset_addNodeToSort(const TNode *node) { addNodeToSort(node); }

bool Nodeset_getSortedNodes(void* userContext, addNodeCb callback) {

    printf("--- namespace table ---\n");
    printf("FileIdx ServerIdx URI\n");
    for(size_t fileIndex = 0; fileIndex < nodeset->namespaceTable->size; fileIndex++) {
        printf("%zu\t%zu\t%s\n", fileIndex, nodeset->namespaceTable->ns[fileIndex].idx,
               nodeset->namespaceTable->ns[fileIndex].name);
    }

    if(!sort(Nodeset_addNode))
    {
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
    }
    return true;
}

void Nodeset_cleanup() {
    Nodeset *n = nodeset;
    // free chars
    for(size_t cnt = 0; cnt < n->charsSize; cnt++) {
        free_const(n->countedChars[cnt]);
    }
    free(n->countedChars);

    // free refs
    for(size_t cnt = 0; cnt < n->refsSize; cnt++) {
        free_const(n->countedRefs[cnt]);
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
            free_const(n->nodes[cnt]->nodes[nodeCnt]);
        }
        free((void *)n->nodes[cnt]->nodes);
        free((void *)n->nodes[cnt]);
    }

    free(n->namespaceTable->ns);
    free(n->namespaceTable);
    free(n);
}

bool isHierachicalReference(const Reference *ref) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(!strcmp(ref->refType.idString, nodeset->hierachicalRefs[i])) {
            return true;
        }
    }
    return false;
}