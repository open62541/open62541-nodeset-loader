/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "Node.h"
#include "DataTypeNode.h"
#include <stdlib.h>
#include "../Value.h"

TNode *Node_new(TNodeClass nodeClass)
{
    TNode *node = NULL;
    switch (nodeClass)
    {
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
    case NODECLASS_VIEW:
        node = (TNode *)calloc(1, sizeof(TViewNode));
        break;
    }
    if(!node)
    {
        return NULL;
    }
    return node;
}

static void deleteRef(Reference *ref)
{
    while (ref)
    {
        Reference *tmp = ref->next;
        free(ref);
        ref = tmp;
    }
}

void Node_delete(TNode *node)
{
    deleteRef(node->hierachicalRefs);
    deleteRef(node->nonHierachicalRefs);
    if (node->nodeClass == NODECLASS_DATATYPE)
    {
        DataTypeNode_clear((TDataTypeNode *)node);
    }
    if(node->nodeClass == NODECLASS_VARIABLE)
    {
        TVariableNode* varNode = (TVariableNode*)node;
        free(varNode->refToTypeDef);
        if(varNode->value)
        {
            Value_delete(varNode->value);
        }
    }
    if(node->nodeClass==NODECLASS_OBJECT)
    {
        TObjectNode *objNode = (TObjectNode *)node;
        free(objNode->refToTypeDef);
    }
    free(node);
}
