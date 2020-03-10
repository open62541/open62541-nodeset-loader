#include "sort.h"
#include "nodesetLoader.h"
#include <check.h>
#include <stdio.h>

static const TNode* sortedNodes[100];
static int sortedNodesCnt = 0;

struct Nodeset;

static void sortCallback(struct Nodeset* nodeset, TNode *node) 
{ 
    printf("%s\n", node->id.idString);
    sortedNodes[sortedNodesCnt] = node;
    sortedNodesCnt++;
}

START_TEST(singleNode) {
    sortedNodesCnt = 0;
    init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";

    addNodeToSort(&a);
    sort(NULL, sortCallback);
    ck_assert_int_eq(sortedNodesCnt, 1);
}
END_TEST

START_TEST(sortNodes) {
    sortedNodesCnt = 0;
    init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";
    TNode b;
    b.hierachicalRefs = NULL;
    b.id.idString = "nodeB";
    TNode c;
    c.hierachicalRefs = NULL;
    c.id.idString = "nodeC";

    addNodeToSort(&a);
    addNodeToSort(&b);
    addNodeToSort(&c);
    sort(NULL, sortCallback);
    ck_assert_int_eq(sortedNodesCnt, 3);
}
END_TEST

// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs_1) {
    sortedNodesCnt = 0;
    init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = "nodeB";

    addNodeToSort(&b);
    addNodeToSort(&a);
    sort(NULL, sortCallback);
    ck_assert_int_eq(sortedNodesCnt, 2);
    ck_assert_str_eq(sortedNodes[0]->id.idString, "nodeA");
    ck_assert_str_eq(sortedNodes[1]->id.idString, "nodeB");
}
END_TEST

// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs_2) {
    sortedNodesCnt = 0;
    init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.idString = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = "nodeB";

    addNodeToSort(&a);
    addNodeToSort(&b);
    sort(NULL, sortCallback);
    ck_assert_int_eq(sortedNodesCnt, 2);
    ck_assert_str_eq(sortedNodes[0]->id.idString, "nodeA");
    ck_assert_str_eq(sortedNodes[1]->id.idString, "nodeB");
}
END_TEST

// cycle nodeB -> nodeA and NodeA -> NodeB
// expect: cycle detection
START_TEST(cycle) {
    sortedNodesCnt = 0;
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

    addNodeToSort(&b);
    addNodeToSort(&a);
    sort(NULL, sortCallback);
}
END_TEST

START_TEST(empty) {
    init();
    sort(NULL, sortCallback);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, singleNode);
    tcase_add_test(tc, sortNodes);
    tcase_add_test(tc, nodeWithRefs_1);
    tcase_add_test(tc, nodeWithRefs_2);
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