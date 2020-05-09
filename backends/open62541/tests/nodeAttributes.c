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

START_TEST(Server_ImportNodeset)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
}
END_TEST

START_TEST(readDescription)
{
    UA_LocalizedText description;
    UA_LocalizedText_init(&description);
    UA_StatusCode retval = UA_Server_readDescription(
        server, UA_NODEID_NUMERIC(2, 5003), &description);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_String loc = UA_STRING("de");
    UA_String text = UA_STRING(
        "Eine komplett unnötige Beschreibung, die nur Speicher braucht und "
        "sonst keine semantischen Infos mit sich bringt, der Standard will es "
        "aber so, darum machen wir es so ohne lange nachzudenken.");
    ck_assert(UA_String_equal(&loc, &description.locale));
    ck_assert(UA_String_equal(&text, &description.text));
    UA_LocalizedText_clear(&description);
}
END_TEST

START_TEST(referenceType)
{
    UA_LocalizedText inverseName;
    UA_LocalizedText_init(&inverseName);
    UA_StatusCode retval = UA_Server_readInverseName(
        server, UA_NODEID_NUMERIC(2, 4002), &inverseName);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_String loc = UA_STRING("en");
    UA_String text = UA_STRING("HeatingZoneOf");
    ck_assert(UA_String_equal(&loc, &inverseName.locale));
    ck_assert(UA_String_equal(&text, &inverseName.text));
    UA_LocalizedText_clear(&inverseName);

    UA_Boolean isSymmetric = UA_FALSE;
    retval = UA_Server_readSymmetric(server, UA_NODEID_NUMERIC(2, 4003),
                                     &isSymmetric);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    ck_assert(isSymmetric == UA_TRUE);

    retval = UA_Server_readSymmetric(server, UA_NODEID_NUMERIC(2, 4002),
                                     &isSymmetric);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    ck_assert(isSymmetric == UA_FALSE);
}
END_TEST

START_TEST(dataType)
{
    UA_Boolean isAbstract = UA_FALSE;
    UA_StatusCode retval = UA_Server_readIsAbstract(server, UA_NODEID_NUMERIC(2, 3002),
                                     &isAbstract);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    ck_assert(isAbstract == UA_TRUE);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("attributes");
    TCase *tc_server = tcase_create("attributes");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_ImportNodeset);
    tcase_add_test(tc_server, readDescription);
    tcase_add_test(tc_server, referenceType);
    tcase_add_test(tc_server, dataType);
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
