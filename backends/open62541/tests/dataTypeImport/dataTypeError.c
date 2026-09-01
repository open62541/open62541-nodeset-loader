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

START_TEST(dataTypeRegistrationErrorIsPropagated) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_UInt16 namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/invalid-datatype-member/");

    ck_assert(!NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeId typeId = UA_NODEID_NUMERIC(namespaceIndex, 3001);
    ck_assert_ptr_null(UA_Server_findDataType(server, &typeId));
    UA_Server_delete(server);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("DataType Error");
    TCase *testCase = tcase_create("datatype error propagation");
    tcase_add_loop_test(testCase, dataTypeRegistrationErrorIsPropagated, 0, 1);
    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_invalidDataTypeMember.xml>\n",
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