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
#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
    const UA_DataTypeArray *customTypes =
        UA_Server_getConfig(server)->customDataTypes;
#endif
    UA_Server_delete(server);
#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
    NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif
}

START_TEST(Server_loadNodeset)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    //datatypes
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 3002)) ==
              UA_NODECLASS_DATATYPE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 3003)) ==
              UA_NODECLASS_DATATYPE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 3004)) ==
              UA_NODECLASS_DATATYPE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 3005)) ==
              UA_NODECLASS_DATATYPE);

    //variables
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6002))==UA_NODECLASS_VARIABLE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6003)) ==
              UA_NODECLASS_VARIABLE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6004)) ==
              UA_NODECLASS_VARIABLE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6005)) ==
              UA_NODECLASS_VARIABLE);

    UA_Variant var;
    UA_Variant_init(&var);
    UA_StatusCode retval =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 6002), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(*(UA_Boolean*)var.data == UA_TRUE);
    UA_Variant_clear(&var);
    retval =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 6004), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_NodeId test = UA_NODEID_NUMERIC(2, 234);
    ck_assert(UA_NodeId_equal(&test, (UA_NodeId*)var.data));
    UA_Variant_clear(&var);
    retval =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 6005), &var);
    ck_assert(*(UA_Int32*)var.data == 42);

    UA_Variant_clear(&var);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadNodeset);
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
