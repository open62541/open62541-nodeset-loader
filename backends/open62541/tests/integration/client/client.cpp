#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "sort_utils.h"
#include "utils.h"
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

using namespace std;

// TODO: check return value of recursive function

UA_Boolean BrowseServerAddressspace(UA_Client *pClient,
                                    const UA_NodeId &StartId,
                                    UA_ByteString *pContinuationPoint,
                                    TNodeIdContainer &NodeIdContainer);

UA_Boolean DoBrowseRecursive(UA_Client *pClient, const UA_NodeId &ParentId,
                             const size_t BrowseResultSize,
                             UA_BrowseResult *pBrowseResults,
                             TNodeIdContainer &NodeIdContainer)
{
    UA_NodeId NextId;
    UA_NodeId_init(&NextId);

    for (size_t i = 0; i < BrowseResultSize; ++i)
    {
        if (pBrowseResults[i].statusCode == UA_STATUSCODE_GOOD)
        {
            for (size_t refIdx = 0; refIdx < pBrowseResults[i].referencesSize;
                 refIdx++)
            {
                UA_ReferenceDescription *ref =
                    &(pBrowseResults[i].references[refIdx]);

                // copy id to container
                UA_NodeId *pId = UA_NodeId_new();
                if (pId == 0)
                {
                    cout
                        << "DoBrowseRecursive() Error: Memory allocation failed"
                        << endl;
                    return UA_FALSE;
                }
                if (UA_NodeId_copy(&ref->nodeId.nodeId, pId) !=
                    UA_STATUSCODE_GOOD)
                {
                    cout << "DoBrowseRecursive() Error: UA_NodeId_copy failed"
                         << endl;
                    return UA_FALSE;
                }
                NodeIdContainer.push_back(pId);

                // check continuation point
                UA_ByteString *ptmpContinuationPoint = 0;
                if ((pBrowseResults[i].continuationPoint.length > 0) &&
                    ((pBrowseResults[i].referencesSize - 1) == refIdx))
                { // use continuation point after reading all current references
                    // otherwise the sequence is not ascending - confusing

                    ptmpContinuationPoint = UA_ByteString_new();
                    UA_ByteString_init(ptmpContinuationPoint);
                    UA_ByteString_copy(&pBrowseResults[i].continuationPoint,
                                       ptmpContinuationPoint);

                    // if we use a continuation point, the ParentId should not
                    // change - ?? really - check continuation point impl!!!!
                    UA_NodeId_copy(&ParentId, &NextId);
                }
                else
                {
                    UA_NodeId_copy(&ref->nodeId.nodeId, &NextId);
                }

                BrowseServerAddressspace(pClient, NextId, ptmpContinuationPoint,
                                         NodeIdContainer);
                UA_NodeId_clear(&NextId);
            }
        }
        else
        {
            cout << "DoBrowseRecursive() Error: browse result statuscode is bad"
                 << endl;
            return UA_FALSE;
        }
    }

    return UA_TRUE;
}

// Browse full server address space and add all NodeIds to container
UA_Boolean BrowseServerAddressspace(UA_Client *pClient,
                                    const UA_NodeId &StartId,
                                    UA_ByteString *pContinuationPoint,
                                    TNodeIdContainer &NodeIdContainer)
{
    assert(pClient != 0);
    UA_Boolean ret = UA_TRUE;

    if (pContinuationPoint != 0)
    {
        UA_BrowseNextRequest bNextReq;
        UA_BrowseNextRequest_init(&bNextReq);
        bNextReq.continuationPointsSize = 1;
        bNextReq.continuationPoints = pContinuationPoint;
        bNextReq.releaseContinuationPoints = false;
        UA_BrowseNextResponse bNextResp =
            UA_Client_Service_browseNext(pClient, bNextReq);

        ret = DoBrowseRecursive(pClient, StartId, bNextResp.resultsSize,
                                bNextResp.results, NodeIdContainer);

        UA_BrowseNextRequest_deleteMembers(&bNextReq);
        UA_BrowseNextResponse_deleteMembers(&bNextResp);
    }
    else
    {
        UA_BrowseRequest bReq;
        UA_BrowseRequest_init(&bReq);
        bReq.requestedMaxReferencesPerNode = 0;
        bReq.nodesToBrowseSize = 1;
        bReq.nodesToBrowse = UA_BrowseDescription_new();
        UA_NodeId_copy(&StartId, &(bReq.nodesToBrowse[0].nodeId));
        bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
        bReq.nodesToBrowse[0].includeSubtypes = UA_TRUE;
        bReq.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(
            0, UA_NS0ID_HIERARCHICALREFERENCES); // TODO: browse all references
        bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
        UA_BrowseResponse bResp = UA_Client_Service_browse(pClient, bReq);

        ret = DoBrowseRecursive(pClient, StartId, bResp.resultsSize,
                                bResp.results, NodeIdContainer);

        UA_BrowseRequest_deleteMembers(&bReq);
        UA_BrowseResponse_deleteMembers(&bResp);
    }
    return ret;
}

int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        cout << "Error: not enough arguments" << endl;
        cout << "command help = integrationClient Server-IP Server-Port "
                "Output-File-Path [Start-NodeId]"
             << endl;
        return EXIT_FAILURE;
    }

    string IP = string(argv[1]);
    string Port = string(argv[2]);
    string FilePath = string(argv[3]);

    /* Visual Studio code debugging with arguments does not work ...
    string IP = "localhost";
    string Port = "4840";
    string FilePath = "Output.txt";
    */
    cout << "Test output file path = '" << FilePath << "'" << endl;

    UA_NodeId StartId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER); // default
    if (argc == 5)
    {
        cout << "Browse Start-NodeId = '" << string(argv[4]) << "'" << endl;
        UA_NodeId_init(&StartId);
        UA_String strId = UA_STRING_ALLOC(argv[4]);
        UA_StatusCode uStatus = UA_NodeId_parse(&StartId, strId);
        UA_String_clear(&strId);
        if (uStatus != UA_STATUSCODE_GOOD)
        {
            cout << "Argument [Start-NodeId] syntax is invalid" << endl;
            UA_NodeId_clear(&StartId);
            return EXIT_FAILURE;
        }
    }

    cout << "Connecting to server: " << IP << ":" << Port << endl;

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    // server may need some time to start
    // try connection establishment multiple times
    const size_t cNoOfReconnectTries = 10;
    size_t i = 0;
    UA_StatusCode retval = UA_STATUSCODE_BADNOTCONNECTED;
    do
    {
        cout << "Try to connect ..." << endl;
        retval =
            UA_Client_connect(client, ("opc.tcp://" + IP + ":" + Port).c_str());
        this_thread::sleep_for(std::chrono::seconds(1));
        i++;
    } while ((retval != UA_STATUSCODE_GOOD) && (i < cNoOfReconnectTries));

    if (retval != UA_STATUSCODE_GOOD)
    {
        cout << "Error: connection could not be established" << endl;
        UA_NodeId_clear(&StartId);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    TNodeIdContainer NodeIdContainer;
    NodeIdContainer.clear();
    ofstream oFileStream;

    if (BrowseServerAddressspace(client, StartId, 0, NodeIdContainer) ==
        UA_TRUE)
    {
        // sort container
        sort(NodeIdContainer.begin(), NodeIdContainer.end(), SortNodeId);

        // print all nodeIds to file
        oFileStream.open(FilePath, ios::binary | ios::trunc);
        if ((oFileStream.is_open()) && (oFileStream.good()))
        {
            for (size_t i = 0; i < NodeIdContainer.size(); i++)
            {
                if (PrintNode(client, *NodeIdContainer[i], oFileStream) ==
                    UA_FALSE)
                {
                    ret = EXIT_FAILURE;
                    break;
                }
                oFileStream << endl;
            }
        }
        else
        {
            cout << "Error: Could not open file. Path = '" << FilePath << "'"
                 << endl;
            ret = EXIT_FAILURE;
        }
    }
    else
    {
        cout << "Error: BrowseServerAddressspace() failed" << endl;
        ret = EXIT_FAILURE;
    }

    for (size_t i = 0; i < NodeIdContainer.size(); i++)
    {
        UA_NodeId_delete(NodeIdContainer[i]);
    }
    oFileStream.close();
    UA_NodeId_clear(&StartId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    cout << "Test finished: " << ((ret == EXIT_SUCCESS) ? "success" : "failure")
         << endl;

    return ret;
}
