/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"

int addNamespace(void* userContext, const char* uri)
{
    return 1;
}

void addNode(void* userContext, const TNode* node)
{
    printf("NodeId: %s BrowseName: %s DisplayName: %s\n", node->id.id,
    node->browseName.name, node->displayName);

    switch (node->nodeClass)
    {
        case NODECLASS_OBJECT:
            printf("\tparentNodeId: %s\n", ((const TObjectNode *)node)->parentNodeId.id);
            printf("\teventNotifier: %s\n", ((const TObjectNode *)node)->eventNotifier);
            break;
        case NODECLASS_VARIABLE:
            printf("\tparentNodeId: %s\n", ((const TVariableNode
    *)node)->parentNodeId.id);
            printf("\tdatatype: %s\n", ((const TVariableNode *)node)->datatype.id);
            printf("\tvalueRank: %s\n", ((const TVariableNode *)node)->valueRank);
            printf("\tarrayDimensions: %s\n", ((const TVariableNode *)node)->valueRank);
            break;
        case NODECLASS_OBJECTTYPE:
            break;
        case NODECLASS_DATATYPE:
            break;
        case NODECLASS_METHOD:
            break;
        case NODECLASS_REFERENCETYPE:
        case NODECLASS_VARIABLETYPE:
            break;

    }
    Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef)
    {
        printf("\treftype: %s target: %s\n", hierachicalRef->refType.id,
    hierachicalRef->target.id);
        hierachicalRef = hierachicalRef->next;
    }

    Reference *nonHierRef = node->nonHierachicalRefs;
    while (nonHierRef)
    {
        printf("\treftype: %s target: %s\n", nonHierRef->refType.id,
    nonHierRef->target.id);
        nonHierRef = nonHierRef->next;
    }
    
}

struct Value *Value_new(const TNode *node) {return NULL;}
void Value_start(Value *val, const char *localname) {}
void Value_end(Value *val, const char *localname, char *value) {}
void Value_finish(Value *val) {}
void Value_delete(Value **val) {}