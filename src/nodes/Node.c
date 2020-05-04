#include "Node.h"
#include "DataTypeNode.h"
#include <assert.h>
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
    }
    assert(node);
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
        if(varNode->value)
        {
            Value_delete(varNode->value);
        }
    }
    free(node);
}
