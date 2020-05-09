#include "CharAllocator.h"
#include <check.h>

START_TEST(simpleMalloc)
{
    CharArenaAllocator *a = (CharArenaAllocator *)CharArenaAllocator_new(100);
    char *val = CharArenaAllocator_malloc(a, 100);
    memset(val, 0, 100);
    CharArenaAllocator_delete(a);
}
END_TEST

START_TEST(simpleMalloc2)
{
    CharArenaAllocator *a = (CharArenaAllocator *)CharArenaAllocator_new(100);
    char *val = CharArenaAllocator_malloc(a, 50);
    char *val2 = CharArenaAllocator_malloc(a, 50);
    char *val3 = CharArenaAllocator_malloc(a, 50);
    CharArenaAllocator_delete(a);
}
END_TEST

START_TEST(simpleRealloc)
{
    CharArenaAllocator *a = (CharArenaAllocator *)CharArenaAllocator_new(100);
    char *val = CharArenaAllocator_malloc(a, 50);
    memcpy(val, "alsökdjfaösldfj", strlen("alsökdjfaösldfj"));
    char *val2 = CharArenaAllocator_realloc(a, 50);
    ck_assert(val == val2);
    ck_assert(!strcmp(val, "alsökdjfaösldfj"));
    CharArenaAllocator_delete(a);
}
END_TEST

START_TEST(simpleRealloc2)
{
    CharArenaAllocator *a = (CharArenaAllocator *)CharArenaAllocator_new(100);
    char *val = CharArenaAllocator_malloc(a, 50);
    val = CharArenaAllocator_malloc(a, 40);
    val = CharArenaAllocator_malloc(a, 10);
    memcpy(val, "123", 3);
    char *val2 = CharArenaAllocator_realloc(a, 3);
    CharArenaAllocator_delete(a);
}
END_TEST

START_TEST(overcommit)
{
    CharArenaAllocator *a = (CharArenaAllocator *)CharArenaAllocator_new(100);
    char *val = CharArenaAllocator_malloc(a, 200);
    memset(val, 0, 200);
    CharArenaAllocator_delete(a);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, simpleMalloc);
    tcase_add_test(tc, simpleMalloc2);
    tcase_add_test(tc, simpleRealloc);
    tcase_add_test(tc, simpleRealloc2);
    tcase_add_test(tc, overcommit);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
