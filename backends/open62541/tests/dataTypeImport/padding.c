/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "check.h"
#include "unistd.h"

#include "../testHelper.h"
#include "../../src/padding.h"


static void setup(void)
{
   
}

static void teardown(void)
{

}

START_TEST(noOffset)
{
    ck_assert(getPadding(4, 0)==0);
    ck_assert(getPadding(8, 0)==0);
    ck_assert(getPadding(1, 0)==0);
    ck_assert(getPadding(2, 0) == 0);
}
END_TEST

START_TEST(OneByteOffset)
{
    ck_assert(getPadding(4, 1) == 3);
    ck_assert(getPadding(8, 1) == 7);
    ck_assert(getPadding(1, 1) == 0);
    ck_assert(getPadding(2, 1) == 1);
}
END_TEST

START_TEST(TwoByteOffset)
{
    ck_assert(getPadding(4, 2) == 2);
    ck_assert(getPadding(8, 2) == 6);
    ck_assert(getPadding(1, 2) == 0);
    ck_assert(getPadding(2, 2) == 0);
}
END_TEST

START_TEST(FourByteOffset)
{
    ck_assert(getPadding(4, 4) == 0);
    ck_assert(getPadding(8, 4) == 4);
    ck_assert(getPadding(1, 4) == 0);
    ck_assert(getPadding(2, 4) == 0);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("padding Test");
    TCase *tc_server = tcase_create("padding Test");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, noOffset);
    tcase_add_test(tc_server, OneByteOffset);
    tcase_add_test(tc_server, TwoByteOffset);
    tcase_add_test(tc_server, FourByteOffset);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
