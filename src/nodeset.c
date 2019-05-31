/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"

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
        id.nsIdx = namespaces[id.nsIdx].idx;
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

void setupNodeset() {
    nodeset = malloc(sizeof(Nodeset));
    nodeset->aliasArray = malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->countedRefs = malloc(sizeof(Reference *) * MAX_REFCOUNTEDREFS);
    nodeset->refsSize = 0;
    nodeset->countedChars = malloc(sizeof(char *) * MAX_REFCOUNTEDCHARS);
    nodeset->charsSize = 0;
    nodeset->nodes[NODECLASS_OBJECT] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECT]->nodes = malloc(sizeof(TNode *) * MAX_OBJECTS);
    nodeset->nodes[NODECLASS_OBJECT]->cnt = 0;
    nodeset->nodes[NODECLASS_VARIABLE] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLE]->nodes = malloc(sizeof(TNode *) * MAX_VARIABLES);
    nodeset->nodes[NODECLASS_VARIABLE]->cnt = 0;
    nodeset->nodes[NODECLASS_METHOD] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_METHOD]->nodes = malloc(sizeof(TNode *) * MAX_METHODS);
    nodeset->nodes[NODECLASS_METHOD]->cnt = 0;
    nodeset->nodes[NODECLASS_OBJECTTYPE] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes = malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt = 0;
    nodeset->nodes[NODECLASS_DATATYPE] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_DATATYPE]->nodes =
        malloc(sizeof(TNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_DATATYPE]->cnt = 0;
    nodeset->nodes[NODECLASS_REFERENCETYPE] = malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes =
        malloc(sizeof(TNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt = 0;
}

void cleanupNodeset() {
    Nodeset *n = nodeset;
    // free chars
    for(size_t cnt = 0; cnt < n->charsSize; cnt++) {
        free((void *)n->countedChars[cnt]);
    }
    free(n->countedChars);

    // free refs
    for(size_t cnt = 0; cnt < n->refsSize; cnt++) {
        free((void *)n->countedRefs[cnt]);
    }
    free(n->countedRefs);

    // free alias
    for(size_t cnt = 0; cnt < n->aliasSize; cnt++) {
        free(n->aliasArray[cnt]);
    }
    free(n->aliasArray);

    for(size_t cnt = 0; cnt < 6; cnt++) {
        size_t storedNodes = n->nodes[cnt]->cnt;
        for(size_t nodeCnt = 0; nodeCnt < storedNodes; nodeCnt++) {
            free((void *)n->nodes[cnt]->nodes[nodeCnt]);
        }
        free((void *)n->nodes[cnt]->nodes);
        free((void *)n->nodes[cnt]);
    }

    // free namespacetable, nodeset
    free(n->namespaceTable->namespace);
    free(n->namespaceTable);
    free(n);
}