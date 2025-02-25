/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"

#include "testHelper.h"
#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

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
#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
    const UA_DataTypeArray *customTypes =
        UA_Server_getConfig(server)->customDataTypes;
#endif
    UA_Server_delete(server);
#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
    NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif
}

START_TEST(references_typeDefinitionId)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));

    UA_NodeId baseDataVariableTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_NodeId typeId = getTypeDefinitionId(server, UA_NODEID_NUMERIC(2, 6012));
    ck_assert(UA_NodeId_equal(&baseDataVariableTypeId, &typeId));

    UA_NodeId folderTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE);
    typeId = getTypeDefinitionId(server, UA_NODEID_NUMERIC(2, 5002));
    ck_assert(UA_NodeId_equal(&folderTypeId, &typeId));
}
END_TEST

START_TEST(forwardReferences)
{
    // both nodes are there
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6002)) == UA_NODECLASS_OBJECT);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6003)) == UA_NODECLASS_OBJECT);
    ck_assert(hasReference(server, UA_NODEID_NUMERIC(2, 6002), UA_NODEID_NUMERIC(2, 6003), 
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_BROWSEDIRECTION_FORWARD));
}
END_TEST

START_TEST(forwardReferences_otherWayRound)
{
    // both nodes are there
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 7002)) ==
              UA_NODECLASS_OBJECT);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 7003)) ==
              UA_NODECLASS_OBJECT);
    ck_assert(hasReference(
        server, UA_NODEID_NUMERIC(2, 7002), UA_NODEID_NUMERIC(2, 7003),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_BROWSEDIRECTION_FORWARD));
}
END_TEST

START_TEST(forwardReferences_EURange)
{
    // both nodes are there
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 7006)) ==
              UA_NODECLASS_OBJECT);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 7005)) ==
              UA_NODECLASS_VARIABLE);
    ck_assert(hasReference(
        server, UA_NODEID_NUMERIC(2, 7006), UA_NODEID_NUMERIC(2, 7005),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), UA_BROWSEDIRECTION_FORWARD));
}
END_TEST

START_TEST(forwardReferences_EURange_newTypeDefRef)
{
    // both nodes are there
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 8002)) ==
              UA_NODECLASS_REFERENCETYPE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 8003)) ==
              UA_NODECLASS_VARIABLE);
    ck_assert(hasReference(server, UA_NODEID_NUMERIC(0, 85),
                           UA_NODEID_NUMERIC(2, 8003),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                           UA_BROWSEDIRECTION_FORWARD));
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("newHierachicalReference");
    TCase *tc_server = tcase_create("newHierachicalReference");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, references_typeDefinitionId);
    tcase_add_test(tc_server, forwardReferences);
    tcase_add_test(tc_server, forwardReferences_otherWayRound);
    tcase_add_test(tc_server, forwardReferences_EURange);
    tcase_add_test(tc_server, forwardReferences_EURange_newTypeDefRef);
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
