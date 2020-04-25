#include "NodeContainer.h"
#include <stdlib.h>
#include "Node.h"
#include <assert.h>

NodeContainer *NodeContainer_new(size_t initialSize)
{
    NodeContainer* container = (NodeContainer*) calloc(1, sizeof(NodeContainer));
    assert(container);
    container->nodes = (TNode**) calloc(initialSize, sizeof(void*));
    assert(container->nodes);
    container->size = 0;
    container->incrementSize = initialSize;
    return container;
}

void NodeContainer_add(NodeContainer* container, TNode *node)
{
    if(container->size == container->incrementSize)
    {
        container->nodes = (TNode**) realloc(container->nodes, (container->size + container->incrementSize) * sizeof(void*) );
        assert(container->nodes);
        container->size += container->incrementSize;
    }
    container->nodes[container->size] = node;
    container->size++;
}

void NodeContainer_delete(NodeContainer *container)
{
    for(size_t i = 0; i < container->size; i++)
    {
        Node_delete(container->nodes[i]);
    }
    free(container->nodes);
    free(container);
}