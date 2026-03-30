/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include <check.h>
#include <stdio.h>

#include <NodesetLoader/backendOpen62541.h>

static UA_Server *server;
static char *nodesetPath = NULL;
static UA_UInt16 nsIdx = 0;

static void setup(void)
{
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    nsIdx = UA_Server_addNamespace(server,
                                   "http://open62541.org/test/emptyStructs/");
    ck_assert_uint_lt(0, nsIdx);

    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
}

static void teardown(void)
{
    UA_Server_delete(server);
}

START_TEST(emptyStructsImport)
{
    const UA_NodeId emptyStructAId = UA_NODEID_NUMERIC(nsIdx, 6001);
    const UA_NodeId emptyStructBId = UA_NODEID_NUMERIC(nsIdx, 6002);
    const UA_NodeId nonEmptyStructCId = UA_NODEID_NUMERIC(nsIdx, 6003);

    const UA_DataType *typeA = UA_Server_findDataType(server, &emptyStructAId);
    const UA_DataType *typeB = UA_Server_findDataType(server, &emptyStructBId);
    const UA_DataType *typeC = UA_Server_findDataType(server,
        &nonEmptyStructCId);

    ck_assert(typeA != NULL);
    ck_assert(typeB != NULL);
    ck_assert(typeC != NULL);
    ck_assert_ptr_ne(typeA, typeB);
    ck_assert_ptr_ne(typeA, typeC);
    ck_assert_ptr_ne(typeB, typeC);

    ck_assert_uint_eq(UA_DATATYPEKIND_STRUCTURE, typeA->typeKind);
    ck_assert_uint_eq(0, typeA->membersSize);
    ck_assert_uint_eq(UA_DATATYPEKIND_STRUCTURE, typeB->typeKind);
    ck_assert_uint_eq(0, typeB->membersSize);
    ck_assert_uint_eq(UA_DATATYPEKIND_STRUCTURE, typeC->typeKind);
    ck_assert_uint_eq(1, typeC->membersSize);
    ck_assert_ptr_ne(NULL, typeC->members);
}
END_TEST

static Suite *testSuite_EmptyStructs(void)
{
    Suite *s = suite_create("EmptyStructs Test");
    TCase *tc_empty = tcase_create("empty structs");
    tcase_add_unchecked_fixture(tc_empty, setup, teardown);
    tcase_add_test(tc_empty, emptyStructsImport);
    suite_add_tcase(s, tc_empty);
    return s;
}

int main(int argc, char *argv[])
{
    if (!(argc > 1)) {
        fprintf(stderr, "Usage: %s <path_to_emptyStructs.xml>\n", argv[0]);
        return EXIT_FAILURE;
    }
    nodesetPath = argv[1];

    Suite *s = testSuite_EmptyStructs();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

