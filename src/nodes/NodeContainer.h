#ifndef NODECONTAINER_H
#define NODECONTAINER_H
#include "Nodeset.h"

struct NodeContainer
{
    TNode **nodes;
    size_t size;
    size_t capacity;
    size_t incrementSize;
    bool owner;
};
typedef struct NodeContainer NodeContainer;

NodeContainer *NodeContainer_new(size_t initialSize, bool owner);
void NodeContainer_delete(NodeContainer *container);
void NodeContainer_add(NodeContainer *container, TNode *node);

#endif
