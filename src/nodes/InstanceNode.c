#include "NodesetLoader/NodesetLoader.h"

bool NodesetLoader_isInstanceNode(const NL_Node *baseNode)
{
    if (baseNode->nodeClass == NODECLASS_VARIABLE ||
        baseNode->nodeClass == NODECLASS_OBJECT ||
        baseNode->nodeClass == NODECLASS_METHOD ||
        baseNode->nodeClass == NODECLASS_VIEW)
    {
        return true;
    }
    return false;
}
