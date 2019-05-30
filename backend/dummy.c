/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"

int addNamespace(const char* uri)
{
    return 1;
}

void addNode(const TNode* node)
{
    return;
    /*
    printf("NodeId: %s BrowseName: %s DisplayName: %s\n", node->id.idString,
    node->browseName, node->displayName);

    switch (node->nodeClass)
    {
        case NODECLASS_OBJECT:
            printf("\tparentNodeId: %s\n", ((TObjectNode *)node)->parentNodeId.idString);
            printf("\teventNotifier: %s\n", ((TObjectNode *)node)->eventNotifier);
            break;
        case NODECLASS_VARIABLE:
            printf("\tparentNodeId: %s\n", ((TVariableNode
    *)node)->parentNodeId.idString);
            printf("\tdatatype: %s\n", ((TVariableNode *)node)->datatype.idString);
            printf("\tvalueRank: %s\n", ((TVariableNode *)node)->valueRank);
            printf("\tarrayDimensions: %s\n", ((TVariableNode *)node)->valueRank);
            break;
        case NODECLASS_OBJECTTYPE:
            break;
        case NODECLASS_DATATYPE:
            break;
        case NODECLASS_METHOD:
            break;
        case NODECLASS_REFERENCETYPE:
            break;

    }
    Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef)
    {
        printf("\treftype: %s target: %s\n", hierachicalRef->refType.idString,
    hierachicalRef->target.idString);
        hierachicalRef = hierachicalRef->next;
    }

    Reference *nonHierRef = node->nonHierachicalRefs;
    while (nonHierRef)
    {
        printf("\treftype: %s target: %s\n", nonHierRef->refType.idString,
    nonHierRef->target.idString);
        nonHierRef = nonHierRef->next;
    }
    */
}