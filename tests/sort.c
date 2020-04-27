#include "Sort.h"
#include <nodesetLoader/NodesetLoader.h>
#include <check.h>
#include <stdio.h>

static const TNode* sortedNodes[100];
static int sortedNodesCnt = 0;

struct Nodeset;

static void sortCallback(struct Nodeset* nodeset, TNode *node) 
{ 
    printf("%s\n", node->id.id);
    sortedNodes[sortedNodesCnt] = node;
    sortedNodesCnt++;
}

START_TEST(singleNode) {
    sortedNodesCnt = 0;
    SortContext* ctx = Sort_init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.nsIdx = 0;
    a.id.id = "nodeA";

    Sort_addNode(ctx, &a);
    Sort_start(ctx, NULL, sortCallback);
    ck_assert(sortedNodesCnt == 1);
    Sort_cleanup(ctx);
}
END_TEST


START_TEST(sortNodes) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TNode a;
    a.hierachicalRefs = NULL;
    a.id.nsIdx =0;
    a.id.id = "nodeA";
    TNode b;
    b.hierachicalRefs = NULL;
    b.id.nsIdx = 0;
    b.id.id = "nodeB";
    TNode c;
    c.hierachicalRefs = NULL;
    c.id.nsIdx = 0;
    c.id.id = "nodeC";

    Sort_addNode(ctx, &a);
    Sort_addNode(ctx, &b);
    Sort_addNode(ctx, &c);
    Sort_start(ctx, NULL, sortCallback);
    ck_assert(sortedNodesCnt==3);
    Sort_cleanup(ctx);
}
END_TEST


// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs_1) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.nsIdx=0;
    a.id.id = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.nsIdx = 0;
    b.id.id = "nodeB";

    Sort_addNode(ctx, &b);
    Sort_addNode(ctx, &a);
    Sort_start(ctx, NULL, sortCallback);
    ck_assert(sortedNodesCnt==2);
    ck_assert(!TNodeId_cmp(&sortedNodes[0]->id, &a.id));
    ck_assert(!TNodeId_cmp(&sortedNodes[1]->id, &b.id));
    Sort_cleanup(ctx);
}
END_TEST


// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs_2) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TNode a;

    a.hierachicalRefs = NULL;
    a.id.nsIdx=0;
    a.id.id = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TNode b;
    b.hierachicalRefs = &ref;
    b.id.nsIdx=0;
    b.id.id = "nodeB";

    Sort_addNode(ctx, &a);
    Sort_addNode(ctx, &b);
    Sort_start(ctx, NULL, sortCallback);
    ck_assert(sortedNodesCnt == 2);
    ck_assert(!TNodeId_cmp(&sortedNodes[0]->id, &a.id));
    ck_assert(!TNodeId_cmp(&sortedNodes[1]->id, &b.id));
    Sort_cleanup(ctx);
}
END_TEST

//todo: fix this test, memleak in sort nodes
// cycle nodeB -> nodeA and NodeA -> NodeB
// expect: cycle detection
START_TEST(cycleDetect) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TNode a;
    a.id.nsIdx = 1;
    a.id.id = "nodeA";

    TNode b;
    b.id.id = "nodeB";
    b.id.nsIdx = 1;

    Reference ref_AToB;
    ref_AToB.isForward = false;
    ref_AToB.target = b.id;
    ref_AToB.next = NULL;

    a.hierachicalRefs = &ref_AToB;

    Reference ref_BToA;
    ref_BToA.isForward = false;
    ref_BToA.target = a.id;
    ref_BToA.next = NULL;

    b.hierachicalRefs = &ref_BToA;

    Sort_addNode(ctx, &b);
    Sort_addNode(ctx, &a);
    ck_assert(!Sort_start(ctx, NULL, sortCallback));
    Sort_cleanup(ctx);
}
END_TEST

START_TEST(empty)
{
    SortContext *ctx = Sort_init();
    Sort_start(ctx, NULL, sortCallback);
    Sort_cleanup(ctx);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, singleNode);
    tcase_add_test(tc, sortNodes);
    tcase_add_test(tc, nodeWithRefs_1);
    tcase_add_test(tc, nodeWithRefs_2);
    tcase_add_test(tc, cycleDetect);
    tcase_add_test(tc, empty);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
