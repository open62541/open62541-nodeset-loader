/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) SICK AG (Author: Joerg Fischer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdio.h>

#include <NodesetLoader/backendOpen62541.h>

static char *nodesetPath;

START_TEST(nonHierarchicalInverseRefsDoNotControlOrder) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_UInt16 namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/non-hierarchical-refs/");

    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeId firstId = UA_NODEID_NUMERIC(namespaceIndex, 5001);
    UA_NodeId secondId = UA_NODEID_NUMERIC(namespaceIndex, 5002);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server, firstId, &nodeClass),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server, secondId, &nodeClass),
                      UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("Non-hierarchical References");
    TCase *testCase = tcase_create("sorting");
    tcase_add_test(testCase, nonHierarchicalInverseRefsDoNotControlOrder);
    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_nonHierarchicalRefs.xml>\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    nodesetPath = argv[1];

    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}