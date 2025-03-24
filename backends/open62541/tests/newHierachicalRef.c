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
char *nodesetPath1 = NULL;
char *nodesetPath2 = NULL;

static void setup(void)
{
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

START_TEST(loadNodeset)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath1, NULL));
    ck_assert(NodesetLoader_loadFile(server, nodesetPath2, NULL));
}
END_TEST

START_TEST(newHierachicalRef)
{
    ck_assert(UA_NODECLASS_REFERENCETYPE ==
              getNodeClass(server, UA_NODEID_NUMERIC(2, 4002)));

    ck_assert(UA_NODECLASS_OBJECT == getNodeClass(server, UA_NODEID_NUMERIC(2, 5002)));

    ck_assert(UA_NODECLASS_OBJECT ==
              getNodeClass(server, UA_NODEID_NUMERIC(2, 5004)));

    ck_assert(hasReference(server, UA_NODEID_NUMERIC(2, 5004),
                           UA_NODEID_NUMERIC(2, 5002),
                           UA_NODEID_NUMERIC(2, 4002), UA_BROWSEDIRECTION_INVERSE));

    ck_assert(hasReference(server, UA_NODEID_NUMERIC(2, 5002),
                           UA_NODEID_NUMERIC(2, 5004),
                           UA_NODEID_NUMERIC(2, 4002), UA_BROWSEDIRECTION_FORWARD));
}
END_TEST

// check if node is also added, if there's no hierachical reference on it
START_TEST(noHierachicalRef)
{
    ck_assert(UA_NODECLASS_OBJECT ==
              getNodeClass(server, UA_NODEID_NUMERIC(2, 5003)));
}
END_TEST

START_TEST(otherNamespace)
{
    ck_assert(UA_NODECLASS_OBJECT ==
              getNodeClass(server, UA_NODEID_NUMERIC(3, 5002)));
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("newHierachicalReference");
    TCase *tc_server = tcase_create("newHierachicalReference");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, loadNodeset);
    tcase_add_test(tc_server, newHierachicalRef);
    tcase_add_test(tc_server, noHierachicalRef);
    tcase_add_test(tc_server, otherNamespace);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
    printf("%s", argv[0]);
    if (!(argc > 1))
        return 1;
    nodesetPath1 = argv[1];
    nodesetPath2 = argv[2];
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
