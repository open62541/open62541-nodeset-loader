/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"

char nodeidDump[256];
static char *
printId(const UA_NodeId *id) {
    UA_String myStr = {0};
    UA_NodeId_print(id, &myStr);
    if(myStr.length >= 256)
        myStr.length = 255;
    memcpy(nodeidDump, myStr.data, myStr.length);
    nodeidDump[myStr.length] = 0;
    UA_String_clear(&myStr);
    return nodeidDump;
}

unsigned short _addNamespace(void *userContext, const char *uri) { return 1; }

void dumpNode(void *userContext, const NL_Node *node)
{
    printf("NodeId: %s BrowseName: %s DisplayName: %s\n", printId(&node->id),
           node->browseName.name, node->displayName.text);

    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        printf("\tparentNodeId: %s\n", printId(&((const NL_ObjectNode *)node)->parentNodeId));
        printf("\teventNotifier: %s\n", ((const NL_ObjectNode *)node)->eventNotifier);
        break;
    case NODECLASS_VARIABLE:
        printf("\tparentNodeId: %s\n",
               printId(&((const NL_VariableNode *)node)->parentNodeId));
        printf("\tdatatype: %s\n", printId(&((const NL_VariableNode *)node)->datatype));
        printf("\tvalueRank: %s\n", ((const NL_VariableNode *)node)->valueRank);
        printf("\tarrayDimensions: %s\n",
               ((const NL_VariableNode *)node)->valueRank);
        printf("\tminimumSamplingInterval: %s\n",
               ((const NL_VariableNode *)node)->minimumSamplingInterval);
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
    NL_Reference *hierachicalRef = node->hierachicalRefs;
    while (hierachicalRef)
    {
        printf("\treftype: %s", printId(&hierachicalRef->refType));
        printf(" target: %s\n", printId(&hierachicalRef->target));
        hierachicalRef = hierachicalRef->next;
    }

    NL_Reference *nonHierRef = node->nonHierachicalRefs;
    while (nonHierRef)
    {
        printf("\treftype: %s", printId(&nonHierRef->refType));
        printf(" target: %s\n", printId(&nonHierRef->target));
        nonHierRef = nonHierRef->next;
    }
}
