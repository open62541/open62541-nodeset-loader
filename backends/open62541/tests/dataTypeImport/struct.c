/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "unistd.h"

#include "../testHelper.h"
#include <NodesetLoader/backendOpen62541.h>
#include <dataTypes.h>

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
    cleanupCustomTypes(UA_Server_getConfig(server)->customDataTypes);
    UA_Server_delete(server);
}

START_TEST(Struct_Point)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_NodeId typeId = UA_NODEID_NUMERIC(2, 3002);
    const UA_DataType *type = getCustomDataType(server, &typeId);
    ck_assert(type);

    struct Point
    {
        UA_Int32 x;
        UA_Int32 y;
        UA_Int32 z;
    };

    ck_assert(sizeof(struct Point) == type->memSize);
}
END_TEST

START_TEST(Single_bool)
{
    UA_NodeId typeId = UA_NODEID_NUMERIC(2, 3003);
    const UA_DataType *type = getCustomDataType(server, &typeId);
    ck_assert(type);

    struct Structbool
    {
        UA_Boolean b;
    };

    ck_assert(sizeof(struct Structbool) == type->memSize);
}
END_TEST

START_TEST(NestedPoints)
{
    UA_NodeId typeId = UA_NODEID_NUMERIC(2, 3004);
    const UA_DataType *type = getCustomDataType(server, &typeId);
    ck_assert(type);

    struct Point
    {
        UA_Int32 x;
        UA_Int32 y;
        UA_Int32 z;
    };

    struct NestedPoints
    {
        struct Point A;
        struct Point B;
    };

    ck_assert(sizeof(struct NestedPoints) == type->memSize);
}
END_TEST

START_TEST(CustomTypeAndNs0Nested)
{
    UA_NodeId typeId = UA_NODEID_NUMERIC(2, 3005);
    const UA_DataType *type = getCustomDataType(server, &typeId);
    ck_assert(type);

    struct Point
    {
        UA_Int32 x;
        UA_Int32 y;
        UA_Int32 z;
    };

    struct Mixed
    {
        UA_Boolean valid;
        struct Point B;
    };
    ck_assert(sizeof(struct Mixed) == type->memSize);
}
END_TEST

START_TEST(StructWithArray)
{
    UA_NodeId typeId = UA_NODEID_NUMERIC(2, 3006);
    const UA_DataType *type = getCustomDataType(server, &typeId);
    ck_assert(type);

    struct StructWithArray
    {
        UA_Boolean valid;
        size_t sizeData;
        UA_Int32* data;
    };
    printf("\nsizeof StructWithArray %d\n", sizeof(struct StructWithArray));
    ck_assert(sizeof(struct StructWithArray) == type->memSize);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("datatype Import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Struct_Point);
    tcase_add_test(tc_server, Single_bool);
    tcase_add_test(tc_server, NestedPoints);
    tcase_add_test(tc_server, CustomTypeAndNs0Nested);
    tcase_add_test(tc_server, StructWithArray);
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
