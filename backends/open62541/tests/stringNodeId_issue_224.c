/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "testHelper.h"

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

START_TEST(Server_Issue_224)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));

    UA_Variant var;
    UA_Variant_init(&var);
    ck_assert_uint_eq(
        UA_STATUSCODE_GOOD,
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 6011), &var));
    ck_assert(var.arrayLength==3);
    ck_assert(var.type==&UA_TYPES[UA_TYPES_NODEID]);
    UA_NodeId* ids = (UA_NodeId*)var.data;
    UA_NodeId firstId = UA_NODEID_STRING(0, "someNode");
    UA_NodeId secondId = UA_NODEID_STRING(0, "someNodeButCooler");
    UA_NodeId thirdId = UA_NODEID_STRING(0, "theNodeIsALie");
    ck_assert(UA_NodeId_equal(&firstId, ids));
    ck_assert(UA_NodeId_equal(&secondId, ids+1));
    ck_assert(UA_NodeId_equal(&thirdId, ids+2));
    UA_Variant_clear(&var);

    ck_assert_uint_eq(
        UA_STATUSCODE_GOOD,
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 6012), &var));
    ck_assert(var.type == &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    UA_PublishedVariableDataType *publishedVar =
        (UA_PublishedVariableDataType *) var.data;
    UA_NodeId publishedVariableId = UA_NODEID_STRING(0, "abcdef");
    ck_assert(UA_NodeId_equal(&publishedVariableId, &publishedVar->publishedVariable));
    UA_Variant_clear(&var);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_Issue_224);
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
