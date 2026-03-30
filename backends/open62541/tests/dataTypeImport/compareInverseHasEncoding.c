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
static UA_UInt16 nsIdx  = 0;

static void setup(void)
{
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    
    // First, register the namespace URI BEFORE loading the nodeset
    nsIdx = UA_Server_addNamespace(server,
        "http://open62541.org/test/inverseEncoding/");
    ck_assert_uint_lt(0, nsIdx);
    
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
}

static void teardown(void) { UA_Server_delete(server); }

START_TEST(forwardHasEncodingIds)
{

    /* Namespace index 1 is used for custom namespace in the nodeset */
    const UA_NodeId forwardOnlyTypeId = UA_NODEID_NUMERIC(nsIdx, 5001);
    const UA_NodeId binaryId = UA_NODEID_NUMERIC(nsIdx, 5002);

    const UA_DataType *customStructType =
        UA_Server_findDataType(server, &forwardOnlyTypeId);
    ck_assert_msg(customStructType != NULL,
                  "CustomStructType (ns=x;i=5001) should exist after import");

    ck_assert(UA_NodeId_equal(&customStructType->binaryEncodingId, &binaryId));
}
END_TEST

START_TEST(inverseHasEncodingIds)
{
    const UA_NodeId inverseOnlyTypeId = UA_NODEID_NUMERIC(nsIdx, 5010);
    const UA_NodeId inverseOnlyBinaryId = UA_NODEID_NUMERIC(nsIdx, 5011);

    const UA_DataType *inverseOnlyType =
        UA_Server_findDataType(server, &inverseOnlyTypeId);
    ck_assert_msg(inverseOnlyType != NULL,
                  "InverseOnlyStructType (ns=x;i=5010) should exist after import");

    ck_assert(UA_NodeId_equal(&inverseOnlyType->binaryEncodingId,
                                  &inverseOnlyBinaryId));
}
END_TEST

static Suite *testSuite_InverseHasEncoding(void)
{
    Suite *s = suite_create("InverseHasEncoding Test");
    TCase *tc_inverse = tcase_create("inverse encoding");
    tcase_add_unchecked_fixture(tc_inverse, setup, teardown);
    tcase_add_test(tc_inverse, forwardHasEncodingIds);
    tcase_add_test(tc_inverse, inverseHasEncodingIds);
    suite_add_tcase(s, tc_inverse);
    return s;
}

int main(int argc, char *argv[])
{
    if (!(argc > 1))
    {
        fprintf(stderr, "Usage: %s <path_to_inverseHasEncoding.xml>\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    nodesetPath = argv[1];

    Suite *s = testSuite_InverseHasEncoding();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
