#ifndef _SORT_UTILS_H
#define _SORT_UTILS_H

#include <open62541/types.h>
#include <vector>

typedef std::vector<UA_NodeId *> TNodeIdContainer;

bool SortNodeId(const UA_NodeId *pLhs, const UA_NodeId *pRhs);

#endif
