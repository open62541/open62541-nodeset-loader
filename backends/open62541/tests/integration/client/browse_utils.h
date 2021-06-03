#ifndef _BROWSE_UTILS_H
#define _BROWSE_UTILS_H

#include <open62541/types.h>
#include <vector>

class UA_Client;

typedef struct
{
    UA_NodeId *pReferenceTypeId;
    UA_NodeId *pTargetId;
} TReference;

typedef std::vector<TReference> TReferenceVec;

UA_Boolean BrowseReferences(UA_Client *pClient, const UA_NodeId &Id,
                            TReferenceVec &oReferences);

void FreeReferencesVec(TReferenceVec &References);

UA_Boolean IsSubType(UA_Client *pClient, const UA_NodeId &BaseType,
                     const UA_NodeId &SubType);

#endif // _BROWSE_UTILS_H