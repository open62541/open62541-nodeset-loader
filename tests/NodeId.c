#include "NodesetLoader/NodeId.h"
#include <check.h>

START_TEST(equal)
{
    NL_NodeId a = {0, "abc"};
    NL_NodeId b = {0, "abc"};
    ck_assert(TNodeId_cmp(&a, &b)==0);
    ck_assert(TNodeId_cmp(&b,&a)==0);
    ck_assert(TNodeId_cmp(&a,&a)==0);
}
END_TEST

START_TEST(notEqual_nsIdx)
{
    NL_NodeId a = {0, "abc"};
    NL_NodeId b = {1, "abc"};
    ck_assert(TNodeId_cmp(&a, &b) != 0);
    ck_assert(TNodeId_cmp(&b, &a) != 0);
    ck_assert(TNodeId_cmp(&a, &a) == 0);
}
END_TEST

START_TEST(notEqual_id)
{
    NL_NodeId a = {11, "abc2"};
    NL_NodeId b = {11, "abc"};
    ck_assert(TNodeId_cmp(&a, &b) != 0);
    ck_assert(TNodeId_cmp(&b, &a) != 0);
    ck_assert(TNodeId_cmp(&a, &a) == 0);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, equal);
    tcase_add_test(tc, notEqual_nsIdx);
    tcase_add_test(tc, notEqual_id);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
