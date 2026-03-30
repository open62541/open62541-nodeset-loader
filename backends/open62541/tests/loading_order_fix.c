/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include <NodesetLoader/backendOpen62541.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

UA_Server *server;
char *nodesetPath = NULL;

static void setup(void)
{
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void)
{
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}


START_TEST(Server_LoadingOrderFix)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    size_t nsIndex = 0;
    const UA_StatusCode retVal = UA_Server_getNamespaceByName(server,
        UA_STRING("http://open62541.org/test/loading_order_fix/"), &nsIndex);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    const UA_NodeId typeBId = UA_NODEID_NUMERIC(nsIndex, 3002);
    const UA_DataType *typeB = UA_Server_findDataType(server, &typeBId);
    ck_assert_msg(typeB != NULL, "TypeB (ns=%lu;i=3002) should be found",
        nsIndex);

    const UA_NodeId typeAId = UA_NODEID_NUMERIC(nsIndex, 3001);
    const UA_DataType *typeA = UA_Server_findDataType(server, &typeAId);
    ck_assert_msg(typeA != NULL, "TypeA (ns=%lu;i=3001) should be found",
        nsIndex);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_LoadingOrderFix);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
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

