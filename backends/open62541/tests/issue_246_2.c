/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include <NodesetLoader/backendOpen62541.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

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
    UA_Server_delete(server);
}

START_TEST(Server_Issue_246)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    // Check if [Object Type] nodes exist with a HasHistoricalConfiguration type inverse reference to the Variable nodes.
    UA_NodeClass node_class;
    UA_NodeClass_init(&node_class);
    UA_StatusCode retval = UA_Server_readNodeClass(
        server, UA_NODEID_STRING(3, "History1.HistoricalDataConfiguration"), &node_class);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(node_class, UA_NODECLASS_OBJECT);
    UA_NodeClass_clear(&node_class);

    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeClass_init(&node_class);
    retval = UA_Server_readNodeClass(
        server, UA_NODEID_STRING(3, "History2.HistoricalDataConfiguration"), &node_class);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(node_class, UA_NODECLASS_OBJECT);
    UA_NodeClass_clear(&node_class);

    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeClass_init(&node_class);
    retval = UA_Server_readNodeClass(
        server, UA_NODEID_STRING(3, "History3.HistoricalDataConfiguration"), &node_class);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(node_class, UA_NODECLASS_OBJECT);
    UA_NodeClass_clear(&node_class);


    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeClass_init(&node_class);
    retval = UA_Server_readNodeClass(
        server, UA_NODEID_STRING(3, "History4.HistoricalDataConfiguration"), &node_class);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(node_class, UA_NODECLASS_OBJECT);
    UA_NodeClass_clear(&node_class);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_Issue_246);
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
