/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../src/conversion.h"
#include "check.h"
#include "unistd.h"
#include <open62541/types.h>

#include "testHelper.h"

static void setup(void)
{

}

static void teardown(void)
{

}

START_TEST(dateTime)
{
    UA_DateTime dt = UA_DateTime_fromString("1970-01-01T00:00:00Z");
    ck_assert_int_eq(dt, UA_DATETIME_UNIX_EPOCH);
} 
END_TEST

START_TEST(dateTime2022)
{
    UA_DateTime dt = UA_DateTime_fromString("2022-02-11T19:02:01Z");
    UA_DateTime fromUnix = UA_DateTime_fromUnixTime(1644606121LL);
    ck_assert_int_eq(dt, fromUnix);
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("conversion");
    TCase *tc_server = tcase_create("conversion");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, dateTime);
    tcase_add_test(tc_server, dateTime2022);
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
