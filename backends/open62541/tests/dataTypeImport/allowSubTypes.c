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

static UA_Server *server;
static char *nodesetPath;
static UA_UInt16 namespaceIndex;

static void
setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/allow-subtypes/");
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
}

static void
teardown(void) {
    UA_Server_delete(server);
}

START_TEST(allowSubTypesUsesExtensionObject) {
    UA_NodeId typeId = UA_NODEID_NUMERIC(namespaceIndex, 3001);
    const UA_DataType *type = UA_Server_findDataType(server, &typeId);

    ck_assert_ptr_nonnull(type);
    ck_assert_uint_eq(type->membersSize, 2);
    ck_assert_ptr_eq(type->members[0].memberType,
                     &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_ptr_eq(type->members[1].memberType, &UA_TYPES[UA_TYPES_STRING]);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("AllowSubTypes");
    TCase *testCase = tcase_create("polymorphic datatype members");
    tcase_add_unchecked_fixture(testCase, setup, teardown);
    tcase_add_test(testCase, allowSubTypesUsesExtensionObject);
    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_allowSubTypes.xml>\n", argv[0]);
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