/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include <NodesetLoader/backendOpen62541.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "testHelper.h"

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
    UA_Server_delete(server);
}

struct MyExtension
{
    char *address;
};

void *Extension_new()
{
    struct MyExtension *newExt = calloc(1, sizeof(struct MyExtension));
    if (!newExt)
    {
        return NULL;
    }
    return newExt;
}

void startExtension(void *ext, const char *name, int attrCnt,
                    const char **attributes)
{
}

void endExtension(void *ext, const char *name, const char *value)
{
    if (ext && !strcmp(name, "Address"))
    {
        ((struct MyExtension *)ext)->address =
            calloc(strlen(value) + 1, sizeof(char));
        memcpy(((struct MyExtension *)ext)->address, value, strlen(value));
    }
}

void finishExtension(void *ext) {}

START_TEST(extensions)
{
    NodesetLoader_ExtensionInterface extIf;
    extIf.userContext = NULL;
    extIf.newExtension = (NodesetLoader_newExtensionCb)Extension_new;
    extIf.start = (NodesetLoader_startExtensionCb)startExtension;
    extIf.end = (NodesetLoader_endExtensionCb)endExtension;
    extIf.finish = (NodesetLoader_finishExtensionCb)finishExtension;
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, &extIf));
    ck_assert(UA_NODECLASS_VARIABLE ==
              getNodeClass(server, UA_NODEID_NUMERIC(2, 6002)));

    struct MyExtension *parsedContext = NULL;
    ck_assert(UA_STATUSCODE_GOOD ==
              UA_Server_getNodeContext(server, UA_NODEID_NUMERIC(2, 6002),
                                       (void **)&parsedContext));
    ck_assert(!strcmp(parsedContext->address, "demo.test"));
    free(parsedContext->address);
    free(parsedContext);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("extensions");
    TCase *tc_server = tcase_create("extensions");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, extensions);
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
