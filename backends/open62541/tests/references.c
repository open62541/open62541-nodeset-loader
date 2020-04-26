#include <check.h>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "unistd.h"

#include "testHelper.h"
#include <openBackend.h>

UA_Server *server;
char *nodesetPath = NULL;

static void setup(void)
{
    printf("path to testnodesets %s\n", nodesetPath);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void)
{
    UA_Server_run_shutdown(server);
    cleanupCustomTypes((UA_DataTypeArray *)(uintptr_t)(
        UA_Server_getConfig(server)->customDataTypes));
    UA_Server_delete(server);
}

static UA_NodeId getTypeDefinitionId(UA_Server* server, const UA_NodeId targetId)
{
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = UA_FALSE;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    bd.nodeId = targetId;
    bd.resultMask = UA_BROWSERESULTMASK_TYPEDEFINITION;
    UA_BrowseResult br= UA_Server_browse(server, 10, &bd);
    if(br.statusCode!= UA_STATUSCODE_GOOD || br.referencesSize!=1)
    {
        return UA_NODEID_NULL;
    }
    UA_NodeId id = br.references->nodeId.nodeId;
    UA_BrowseResult_clear(&br);
    return id;
}

START_TEST(references_typeDefinitionId)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));

    UA_NodeId baseDataVariableTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_NodeId typeId = getTypeDefinitionId(server, UA_NODEID_NUMERIC(2, 6012));
    ck_assert(UA_NodeId_equal(&baseDataVariableTypeId, &typeId));

    UA_NodeId folderTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE);
    typeId = getTypeDefinitionId(server, UA_NODEID_NUMERIC(2, 5002));
    ck_assert(UA_NodeId_equal(&folderTypeId, &typeId));
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("newHierachicalReference");
    TCase *tc_server = tcase_create("newHierachicalReference");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, references_typeDefinitionId);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
    printf("%s", argv[0]);
    if (!(argc > 1))
        return 1;
    nodesetPath = argv[1];
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}