/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include "NodesetLoader/NodesetLoader.h"
#include <stdlib.h>

unsigned short addNamespace(void *userContext, const char *uri) { return 1; }

void addNode(void *userContext, const NL_Node *node)
{
    (*((int*)userContext))++;
}

char *nodesetPath = NULL;

static void setup(void)
{

}

static void teardown(void)
{

}

START_TEST(Server_ImportBasicNodeClassTest)
{
    NL_FileContext handler;
    handler.addNamespace = addNamespace;

    NodesetLoader *loader = NodesetLoader_new(NULL, NULL);
    handler.file = nodesetPath;
    ck_assert(NodesetLoader_importFile(loader, &handler));
    ck_assert(NodesetLoader_sort(loader));

    int nodeCount=0;

    for (int i = 0; i < NL_NODECLASS_COUNT; i++)
    {
        NodesetLoader_forEachNode(loader, (NL_NodeClass)i, &nodeCount,
                                  (NodesetLoader_forEachNode_Func)addNode);
    }

    printf("Loaded %i nodes\n", nodeCount);

    NodesetLoader_delete(loader);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_ImportBasicNodeClassTest);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
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
