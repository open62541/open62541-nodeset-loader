#include "sort_utils.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************************/
bool SortNodeId(const UA_NodeId *pLhs, const UA_NodeId *pRhs)
{
    assert(pLhs != 0);
    assert(pRhs != 0);
    if ((pLhs == 0) || (pRhs == 0))
    {
        cout << "Error SortNodeId(): null pointer" << endl;
        return false;
    }

    if (UA_NodeId_order(pLhs, pRhs) <= 0)
    {
        return true;
    }
    return false;
}
