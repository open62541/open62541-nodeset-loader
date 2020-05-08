#ifndef _BROWSE_UTILS_H
#define _BROWSE_UTILS_H

#include <open62541/client_config.h>
#include <open62541/types.h>
#include <vector>

typedef struct
{
    UA_NodeId *pReferenceTypeId;
    UA_NodeId *pTargetId;
} TReference;

typedef std::vector<TReference> TReferenceVec;

UA_Boolean BrowseReferences(UA_Client *pClient, const UA_NodeId &Id,
                            TReferenceVec &oReferences);

void FreeReferencesVec(TReferenceVec &References);

UA_Boolean IsSubType(const UA_NodeId &BaseType, const UA_NodeId &SubType);

#endif // _BROWSE_UTILS_H