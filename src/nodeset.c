/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"

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

Alias *aliasArray[MAX_ALIAS];

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