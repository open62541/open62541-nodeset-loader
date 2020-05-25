#include "browse_utils.h"
#include "operator_ov.h"
#include "sort_utils.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <open62541/client_highlevel.h>
#include <open62541/types.h>

using namespace std;

bool SortReference(const TReference Lhs, const TReference Rhs)
{
    // start sorting with reference type id
    UA_Order order =
        UA_NodeId_order(Lhs.pReferenceTypeId, Rhs.pReferenceTypeId);

    if (order == UA_ORDER_LESS)
    {
        return true;
    }
    if (order == UA_ORDER_MORE)
    {
        return false;
    }

    // reference type Ids are equal
    // sort target Ids
    return SortNodeId(Lhs.pTargetId, Rhs.pTargetId);
}

UA_Boolean BrowseReferences(UA_Client *pClient, const UA_NodeId &Id,
                            TReferenceVec &oReferences)
{
    UA_Boolean ret = UA_TRUE;
    oReferences.clear();

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    UA_NodeId_copy(&Id, &(bReq.nodesToBrowse[0].nodeId));
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bReq.nodesToBrowse[0].includeSubtypes = UA_TRUE;
    bReq.nodesToBrowse[0].referenceTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResponse bResp = UA_Client_Service_browse(pClient, bReq);
    if (bResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        TReference CopyReference; // copy browse results to output vector
        for (size_t i = 0; i < bResp.resultsSize; i++)
        {
            for (size_t refIdx = 0; refIdx < bResp.results[i].referencesSize;
                 refIdx++)
            {
                UA_ReferenceDescription *ref =
                    &bResp.results[i].references[refIdx];

                CopyReference.pReferenceTypeId = UA_NodeId_new();
                assert(CopyReference.pReferenceTypeId !=
                       0); // TODO: proper check
                UA_StatusCode status = UA_NodeId_copy(
                    &ref->referenceTypeId, CopyReference.pReferenceTypeId);
                assert(UA_STATUSCODE_GOOD == status);

                CopyReference.pTargetId = UA_NodeId_new();
                assert(CopyReference.pTargetId != 0); // TODO: proper check
                status = UA_NodeId_copy(&ref->nodeId.nodeId,
                                        CopyReference.pTargetId);
                assert(status == UA_STATUSCODE_GOOD);

                oReferences.push_back(CopyReference);
            }
        }

        // sort references
        sort(oReferences.begin(), oReferences.end(), SortReference);
    }
    else
    {
        cout << "Error BrowseReferences failed. Id = " << Id << endl;
        ret = UA_FALSE;
    }

    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    return ret;
}

void FreeReferencesVec(TReferenceVec &References)
{
    for (size_t i = 0; i < References.size(); i++)
    {
        UA_NodeId_clear(References[i].pTargetId);
        UA_NodeId_delete(References[i].pTargetId);

        UA_NodeId_clear(References[i].pReferenceTypeId);
        UA_NodeId_delete(References[i].pReferenceTypeId);
    }
    References.clear();
}

UA_Boolean IsSubType(UA_Client *pClient, const UA_NodeId &BaseType,
                     const UA_NodeId &SubType)
{
    // search from subtype to basetype (browse inverse)
    UA_Boolean bFoundBaseType = UA_FALSE;

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    UA_NodeId_copy(&SubType, &(bReq.nodesToBrowse[0].nodeId));
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bReq.nodesToBrowse[0].includeSubtypes = UA_TRUE;
    bReq.nodesToBrowse[0].referenceTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResponse bResp = UA_Client_Service_browse(pClient, bReq);
    if ((bResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD) &&
        (bResp.resultsSize == 1))
    {
        if ((bResp.results[0].statusCode == UA_STATUSCODE_GOOD) &&
            (bResp.results[0].referencesSize == 1))
        {
            if (UA_NodeId_equal(&BaseType,
                                &bResp.results[0].references[0].nodeId.nodeId))
            {
                bFoundBaseType = UA_TRUE; // found the base type
            }
            else
            {
                // continue inverse search for base type
                bFoundBaseType =
                    IsSubType(pClient, BaseType,
                              bResp.results[0].references[0].nodeId.nodeId);
            }
        } // else: there are no more references to search
    }
    else
    {
        cout << "Error IsSubType(): BrowseRequest failed" << endl;
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);
    return bFoundBaseType;
}