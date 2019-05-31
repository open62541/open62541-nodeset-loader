#include "sort.h"
#include <check.h>
#include <stdio.h>


static void sortCallback(const TNode *node) { printf("%s\n", node->id.idString); }

START_TEST(sortNodes) {
    init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";
    TNode b;
    b.hierachicalRefs = NULL;
    b.id.idString = "nodeB";

    insertNode(&a);
    insertNode(&b);
    sort(sortCallback);
}
END_TEST

// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs) {
    init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = "nodeB";

    insertNode(&b);
    insertNode(&a);
    sort(sortCallback);
}
END_TEST

// cycle nodeB -> nodeA and NodeA -> NodeB
// expect: cycle detection
START_TEST(cycle) {
    init();

    TNode a;

    TNodeId idb;
    idb.idString = "nodeB";
    idb.nsIdx = 1;
    idb.id = "test";

    Reference refb;
    refb.isForward = false;
    refb.target = idb;
    refb.next = NULL;

    a.hierachicalRefs = &refb;
    a.id.idString = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = "nodeB";

    insertNode(&b);
    insertNode(&a);
    sort(sortCallback);
}
END_TEST

START_TEST(empty) {
    init();
    sort(sortCallback);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, sortNodes);
    tcase_add_test(tc, nodeWithRefs);
    tcase_add_test(tc, cycle);
    tcase_add_test(tc, empty);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}