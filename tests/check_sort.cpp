#include "sort.h"
#include "nodesetLoader.h"
#include <gtest/gtest.h>
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

TEST(sort, singleNode) {
    sortedNodesCnt = 0;
    init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.idString = (char *)"nodeA";

    addNodeToSort(&a);
    sort(NULL, sortCallback);
    ASSERT_EQ(sortedNodesCnt, 1);
}


TEST(sort, sortNodes) {
    sortedNodesCnt = 0;
    init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.idString = (char *)"nodeA";
    TNode b;
    b.hierachicalRefs = NULL;
    b.id.idString = (char *)"nodeB";
    TNode c;
    c.hierachicalRefs = NULL;
    c.id.idString = (char *)"nodeC";

    addNodeToSort(&a);
    addNodeToSort(&b);
    addNodeToSort(&c);
    sort(NULL, sortCallback);
    ASSERT_EQ(sortedNodesCnt, 3);
}


// nodeB -> nodeA
// expect: nodeA, nodeB
TEST(sort, nodeWithRefs_1) {
    sortedNodesCnt = 0;
    init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.idString = (char *)"nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = (char *)"nodeB";

    addNodeToSort(&b);
    addNodeToSort(&a);
    sort(NULL, sortCallback);
    ASSERT_EQ(sortedNodesCnt, 2);
    ASSERT_EQ(sortedNodes[0]->id.idString, "nodeA");
    ASSERT_EQ(sortedNodes[1]->id.idString, "nodeB");
}


// nodeB -> nodeA
// expect: nodeA, nodeB
TEST(sort, nodeWithRefs_2) {
    sortedNodesCnt = 0;
    init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.idString = (char*)"nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = (char*)"nodeB";

    addNodeToSort(&a);
    addNodeToSort(&b);
    sort(NULL, sortCallback);
    ASSERT_EQ(sortedNodesCnt, 2);
    ASSERT_EQ(sortedNodes[0]->id.idString, "nodeA");
    ASSERT_EQ(sortedNodes[1]->id.idString, "nodeB");
}

// cycle nodeB -> nodeA and NodeA -> NodeB
// expect: cycle detection
TEST(sort, cycle) {
    sortedNodesCnt = 0;
    init();

    TNode a;

    TNodeId idb;
    idb.idString = (char*)"nodeB";
    idb.nsIdx = 1;
    idb.id = (char *)"test";

    Reference refb;
    refb.isForward = false;
    refb.target = idb;
    refb.next = NULL;

    a.hierachicalRefs = &refb;
    a.id.idString = (char *)"nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.idString = (char *)"nodeB";

    addNodeToSort(&b);
    addNodeToSort(&a);
    sort(NULL, sortCallback);
}

TEST(sort, empty) {
    init();
    sort(NULL, sortCallback);
}
/*
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
*/