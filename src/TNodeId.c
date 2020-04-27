#include <NodesetLoader/TNodeId.h>
#include <string.h>

int TNodeId_cmp(const TNodeId *id1, const TNodeId *id2)
{
    if (id1->nsIdx == id2->nsIdx)
    {
        return strcmp(id1->id, id2->id);
    }
    if (id1->nsIdx < id2->nsIdx)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}
