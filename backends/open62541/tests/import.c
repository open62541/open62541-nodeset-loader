/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "unistd.h"

#include <openBackend.h>

UA_Server *server;
char* nodesetPath=NULL;

static void setup(void) {
    printf("path to testnodesets %s\n", nodesetPath);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_ImportNodeset) {
    FileContext ctx;
    ctx.callback = Backend_addNode;
    ctx.addNamespace = Backend_addNamespace;
    ctx.userContext = server;
    ctx.file = nodesetPath;
    ValueInterface valIf;
    valIf.userContext = NULL;
    valIf.newValue = Value_new;
    valIf.start = Value_start;
    valIf.end = Value_end;
    valIf.finish = Value_finish;
    valIf.deleteValue = Value_delete;
    ctx.valueHandling = &valIf;
    ck_assert(loadFile(&ctx));
}
END_TEST


START_TEST(Server_ImportNoFile) {
    FileContext ctx;
    ctx.callback = Backend_addNode;
    ctx.addNamespace = Backend_addNamespace;
    ctx.userContext = server;
    ctx.file = "notExisting.xml";
    ValueInterface valIf;
    valIf.userContext = NULL;
    valIf.newValue = Value_new;
    valIf.start = Value_start;
    valIf.end = Value_end;
    valIf.finish = Value_finish;
    valIf.deleteValue = Value_delete;
    ctx.valueHandling = &valIf;
    ck_assert(!loadFile(&ctx));
}
END_TEST


START_TEST(Server_EmptyHandler) {
    ck_assert(!loadFile(NULL));
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_ImportNodeset);
    tcase_add_test(tc_server, Server_ImportNoFile);
    tcase_add_test(tc_server, Server_EmptyHandler);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char*argv[]) {
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
