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
static const char *nodesetPaths[3];

static void
setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
}

static void
teardown(void) {
    UA_Server_delete(server);
}

START_TEST(subtypeOfNamespaceZeroAbstractType) {
    UA_UInt16 namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/abstract-subtype/");
    ck_assert(NodesetLoader_loadFile(server, nodesetPaths[0], NULL));

    UA_NodeId typeId = UA_NODEID_NUMERIC(namespaceIndex, 3001);
    const UA_DataType *type = UA_Server_findDataType(server, &typeId);
    ck_assert_ptr_nonnull(type);
    ck_assert_int_eq(type->typeKind, UA_DATATYPEKIND_BYTESTRING);
    ck_assert_uint_eq(type->memSize, sizeof(UA_ByteString));
}
END_TEST

START_TEST(subtypeOfCustomAbstractType) {
    UA_UInt16 namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/custom-abstract-subtype/");
    ck_assert(NodesetLoader_loadFile(server, nodesetPaths[1], NULL));

    UA_NodeId abstractTypeId = UA_NODEID_NUMERIC(namespaceIndex, 3001);
    const UA_DataType *abstractType =
        UA_Server_findDataType(server, &abstractTypeId);
    ck_assert_ptr_nonnull(abstractType);
    ck_assert_int_eq(abstractType->typeKind, UA_DATATYPEKIND_BYTESTRING);

    UA_NodeId concreteTypeId = UA_NODEID_NUMERIC(namespaceIndex, 3002);
    const UA_DataType *concreteType =
        UA_Server_findDataType(server, &concreteTypeId);
    ck_assert_ptr_nonnull(concreteType);
    ck_assert_int_eq(concreteType->typeKind, UA_DATATYPEKIND_BYTESTRING);
    ck_assert_uint_eq(concreteType->memSize, sizeof(UA_ByteString));
}
END_TEST

START_TEST(subtypeOfBaseDataType) {
    UA_UInt16 namespaceIndex = UA_Server_addNamespace(
        server, "http://open62541.org/test/base-datatype-subtype/");
    ck_assert(NodesetLoader_loadFile(server, nodesetPaths[2], NULL));

    UA_NodeId typeId = UA_NODEID_NUMERIC(namespaceIndex, 3001);
    const UA_DataType *type = UA_Server_findDataType(server, &typeId);
    ck_assert_ptr_nonnull(type);
    ck_assert_int_eq(type->typeKind, UA_DATATYPEKIND_VARIANT);
    ck_assert_uint_eq(type->memSize, sizeof(UA_Variant));
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("Abstract DataType Subtype");
    TCase *testCase = tcase_create("opaque subtype resolution");
    tcase_add_unchecked_fixture(testCase, setup, teardown);
    tcase_add_test(testCase, subtypeOfNamespaceZeroAbstractType);
    tcase_add_test(testCase, subtypeOfCustomAbstractType);
    tcase_add_test(testCase, subtypeOfBaseDataType);
    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(int argc, char *argv[]) {
    if(argc < 4) {
        fprintf(stderr, "Usage: %s <abstract> <custom-abstract> <base>\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    for(size_t i = 0; i < 3; i++)
        nodesetPaths[i] = argv[i + 1];

    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}