#include "nodes/NodeContainer.h"
#include <NodesetLoader/NodesetLoader.h>
#include <check.h>
#include <stdio.h>

static void initNode(TVariableNode* n)
{
    memset(n, 0, sizeof(TVariableNode));
}

START_TEST(newEmptyContainer) {

    NodeContainer* container = NodeContainer_new(100, true);
    NodeContainer_delete(container);
}
END_TEST

START_TEST(ownership) {

    TVariableNode varNode;
    initNode(&varNode);
    
    NodeContainer* container = NodeContainer_new(100, false);
    for(int i=0; i<100; i++)
    {
        NodeContainer_add(container, (NL_Node*)&varNode);
    }
    NodeContainer_delete(container);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, newEmptyContainer);
    tcase_add_test(tc, ownership);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
