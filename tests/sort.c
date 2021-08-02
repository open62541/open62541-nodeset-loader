#include "Sort.h"
#include <NodesetLoader/NodesetLoader.h>
#include <check.h>
#include <stdio.h>

static const NL_Node* sortedNodes[100];
static int sortedNodesCnt = 0;

struct Nodeset;

static void sortCallback(struct Nodeset* nodeset, NL_Node *node) 
{ 
    printf("%s\n", node->id.id);
    sortedNodes[sortedNodesCnt] = node;
    sortedNodesCnt++;
}

static void initNode(TVariableNode* n)
{
    memset(n, 0, sizeof(TVariableNode));
}

START_TEST(singleNode) {
    sortedNodesCnt = 0;
    SortContext* ctx = Sort_init();

    TVariableNode a;
    initNode(&a);
    a.id.id = "nodeA";
    a.nodeClass = NODECLASS_VARIABLE;

    Sort_addNode(ctx, (NL_Node *)&a);
    Sort_start(ctx, NULL, sortCallback, NULL);
    ck_assert(sortedNodesCnt == 1);
    Sort_cleanup(ctx);
}
END_TEST


START_TEST(sortNodes) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TVariableNode a;
    initNode(&a);
    a.id.id = "nodeA";
    a.nodeClass = NODECLASS_VARIABLE;
    TVariableNode b;
    initNode(&b);
    b.id.id = "nodeB";
    b.nodeClass = NODECLASS_VARIABLE;
    TVariableNode c;
    initNode(&c);
    c.id.id = "nodeC";
    c.nodeClass = NODECLASS_VARIABLE;

    Sort_addNode(ctx, (NL_Node*)&a);
    Sort_addNode(ctx, (NL_Node *)&b);
    Sort_addNode(ctx, (NL_Node *)&c);
    Sort_start(ctx, NULL, sortCallback, NULL);
    ck_assert(sortedNodesCnt==3);
    Sort_cleanup(ctx);
}
END_TEST


// nodeB -> nodeA
// expect: nodeA, nodeB
START_TEST(nodeWithRefs_1) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TVariableNode a;
    initNode(&a);
    a.id.id = "nodeA";
    a.nodeClass = NODECLASS_VARIABLE;

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TVariableNode b;
    initNode(&b);
    b.hierachicalRefs = &ref;
    b.id.id = "nodeB";
    b.nodeClass = NODECLASS_VARIABLE;

    Sort_addNode(ctx, (NL_Node *)&b);
    Sort_addNode(ctx, (NL_Node *)&a);
    Sort_start(ctx, NULL, sortCallback, NULL);
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

    TVariableNode a;
    initNode(&a);
    a.id.id = "nodeA";

    Reference ref;
    ref.isForward = false;
    ref.target = a.id;
    ref.next = NULL;

    TVariableNode b;
    initNode(&b);
    b.hierachicalRefs = &ref;
    b.id.nsIdx=0;
    b.id.id = "nodeB";

    Sort_addNode(ctx, (NL_Node *)&a);
    Sort_addNode(ctx, (NL_Node *)&b);
    Sort_start(ctx, NULL, sortCallback, NULL);
    ck_assert(sortedNodesCnt == 2);
    ck_assert(!TNodeId_cmp(&sortedNodes[0]->id, &a.id));
    ck_assert(!TNodeId_cmp(&sortedNodes[1]->id, &b.id));
    Sort_cleanup(ctx);
}
END_TEST

// cycle nodeB -> nodeA and NodeA -> NodeB
// expect: cycle detection
START_TEST(cycleDetect) {
    sortedNodesCnt = 0;
    SortContext *ctx = Sort_init();

    TVariableNode a;
    initNode(&a);
    a.id.nsIdx = 1;
    a.id.id = "nodeA";

    TVariableNode b;
    initNode(&b);
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

    Sort_addNode(ctx, (NL_Node *)&b);
    Sort_addNode(ctx, (NL_Node *)&a);
    ck_assert(!Sort_start(ctx, NULL, sortCallback, NULL));
    Sort_cleanup(ctx);
}
END_TEST

START_TEST(empty)
{
    SortContext *ctx = Sort_init();
    Sort_start(ctx, NULL, sortCallback, NULL);
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
