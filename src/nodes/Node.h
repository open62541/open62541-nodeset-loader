#ifndef NODE_H
#define NODE_H
#include <NodesetLoader/NodesetLoader.h>

TNode *Node_new(TNodeClass nodeClass);
void Node_delete(TNode *node);

#endif
