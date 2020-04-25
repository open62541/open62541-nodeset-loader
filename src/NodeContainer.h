#ifndef NODECONTAINER_H
#define NODECONTAINER_H
#include "Nodeset.h"

struct NodeContainer
{
    TNode **nodes;
    size_t size;
    size_t incrementSize;
};
typedef struct NodeContainer NodeContainer;

NodeContainer* NodeContainer_new(size_t initialSize);
void NodeContainer_delete(NodeContainer* container);
void NodeContainer_add(NodeContainer* container, TNode* node);

#endif