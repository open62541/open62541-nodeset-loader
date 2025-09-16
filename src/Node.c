/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "Node.h"

bool
NodeContainer_init(NodeContainer *container, size_t initialSize) {
    memset(container, 0, sizeof(NodeContainer));

    container->nodes =
        (NL_Node **)calloc(initialSize, sizeof(NL_Node*));
    if(!container->nodes)
        return false;
    container->size = 0;
    container->capacity = initialSize;
    return true;
}

bool
NodeContainer_add(NodeContainer *container, NL_Node *node) {
    if(container->size == container->capacity) {
        container->nodes = (NL_Node **)realloc(
            container->nodes, container->size * 2 * sizeof(void *));
        if(!container->nodes)
            return false;
        container->capacity *= 2;
    }
    container->nodes[container->size] = node;
    container->size++;
    return true;
}

void NodeContainer_remove(NodeContainer *container, size_t index) {
    container->nodes[index] = container->nodes[container->size-1];
    container->size--;
}

void
NodeContainer_clear(NodeContainer *container) {
    free(container->nodes);
    memset(container, 0, sizeof(NodeContainer));
}

NL_Node *
Node_new(NL_NodeClass nodeClass) {
    void *node = NULL;
    switch (nodeClass)
    {
    case NODECLASS_VARIABLE:
        node = calloc(1, sizeof(NL_VariableNode));
        break;
    case NODECLASS_OBJECT:
        node = calloc(1, sizeof(NL_ObjectNode));
        break;
    case NODECLASS_OBJECTTYPE:
        node = calloc(1, sizeof(NL_ObjectTypeNode));
        break;
    case NODECLASS_REFERENCETYPE:
        node = calloc(1, sizeof(NL_ReferenceTypeNode));
        break;
    case NODECLASS_VARIABLETYPE:
        node = calloc(1, sizeof(NL_VariableTypeNode));
        break;
    case NODECLASS_DATATYPE:
        node = calloc(1, sizeof(NL_DataTypeNode));
        break;
    case NODECLASS_METHOD:
        node = calloc(1, sizeof(NL_MethodNode));
        break;
    case NODECLASS_VIEW:
        node = calloc(1, sizeof(NL_ViewNode));
        break;
    }
    if(!node)
        return NULL;
    return (NL_Node*)node;
}

static void
deleteRef(NL_Reference *ref) {
    while (ref) {
        NL_Reference *tmp = ref->next;
        UA_NodeId_clear(&ref->target);
        UA_NodeId_clear(&ref->refType);
        free(ref);
        ref = tmp;
    }
}

void
Node_delete(NL_Node *node) {
    UA_NodeId_clear(&node->id);
    UA_QualifiedName_clear(&node->browseName);
    deleteRef(node->refs);

    if(node->nodeClass == NODECLASS_VARIABLE) {
        NL_VariableNode* varNode = (NL_VariableNode*)node;
        UA_String_clear(&varNode->value);
        UA_NodeId_clear(&varNode->datatype);
    }

    if(node->nodeClass==NODECLASS_OBJECT) {
        NL_ObjectNode *objNode = (NL_ObjectNode *)node;
        UA_NodeId_clear(&objNode->parentNodeId);
    }

    if(node->nodeClass==NODECLASS_DATATYPE) {
        NL_DataTypeNode *dtNode = (NL_DataTypeNode *)node;
        UA_String_clear(&dtNode->typeDefinition);
    }
    free(node);
}
