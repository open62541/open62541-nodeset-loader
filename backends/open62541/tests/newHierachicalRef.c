#include <check.h>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "unistd.h"

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
    UA_Server_delete(server);
}

START_TEST(import_ValueRank)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));

    UA_NodeClass refTypeClass = UA_NODECLASS_DATATYPE;
    UA_Server_readNodeClass(server, UA_NODEID_NUMERIC(2, 4002), &refTypeClass);
    ck_assert(UA_NODECLASS_REFERENCETYPE== refTypeClass);

    UA_Server_readNodeClass(server, UA_NODEID_NUMERIC(2, 5002), &refTypeClass);
    ck_assert(UA_NODECLASS_OBJECT == refTypeClass);

    UA_StatusCode status = UA_Server_readNodeClass(server, UA_NODEID_NUMERIC(2, 5003), &refTypeClass);
    ck_assert(UA_STATUSCODE_GOOD==status);
    ck_assert(UA_NODECLASS_OBJECT == refTypeClass);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("newHierachicalReference");
    TCase *tc_server = tcase_create("newHierachicalReference");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, import_ValueRank);
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