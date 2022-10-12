/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "unistd.h"

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
    const UA_DataTypeArray* customTypes = UA_Server_getConfig(server)->customDataTypes;
    UA_Server_delete(server);
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
    NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif
}

START_TEST(Server_loadNodeset)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 3002))==UA_NODECLASS_DATATYPE);
    ck_assert(getNodeClass(server, UA_NODEID_NUMERIC(2, 6008)) ==
              UA_NODECLASS_VARIABLE);
    struct Point
    {
        UA_Int32 x;
        UA_Int32 y;
        size_t size;
        UA_Int32* scaleFactors;
    };

    UA_Variant var;
    UA_Variant_init(&var);

    UA_StatusCode status = UA_Server_readValue(server, UA_NODEID_NUMERIC(2,6008), &var);
    ck_assert(status == UA_STATUSCODE_GOOD);
    struct Point* p = (struct Point*)var.data;
    ck_assert(p->x==10);
    ck_assert(p->y==20);
    ck_assert(p->size==4);
    ck_assert(p->scaleFactors[3]==23);

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
