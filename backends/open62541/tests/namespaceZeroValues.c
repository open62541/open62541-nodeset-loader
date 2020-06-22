/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "unistd.h"

#include "testHelper.h"
#include <NodesetLoader/backendOpen62541.h>

UA_Server *server;
char* nodesetPath=NULL;

static void setup(void) {
    printf("path to testnodesets %s\n", nodesetPath);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    cleanupCustomTypes(UA_Server_getConfig(server)->customDataTypes);
    UA_Server_delete(server);
}

static UA_UInt16 getNamespaceIndex(const char* uri)
{
    UA_Variant namespaceArray;
    UA_Variant_init(&namespaceArray);
    UA_Server_readValue(server, UA_NODEID_NUMERIC(0, 2255), &namespaceArray);
    UA_UInt16 nsidx = 0;
    for(size_t cnt = 0; cnt < namespaceArray.arrayLength; cnt++) {
        if(!strncmp((char*)((UA_String*)namespaceArray.data)[cnt].data, uri, ((UA_String*)namespaceArray.data)[cnt].length))
        {
            nsidx =(UA_UInt16)cnt;
            break;
        }
    }
    UA_Variant_clear(&namespaceArray);
    return nsidx;
}

START_TEST(Server_LoadNS0Values) {

    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));
    UA_UInt16 nsIdx =
        getNamespaceIndex("http://open62541.com/nodesetimport/tests/namespaceZeroValues");
    ck_assert_uint_gt(nsIdx, 0);
    UA_Variant var;
    UA_Variant_init(&var);
    //scalar double
    UA_StatusCode retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1003), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_DOUBLE);
    ck_assert(*(UA_Double *)var.data - 3.14 < 0.01);
    UA_Variant_clear(&var);
    //array of Uint32
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1004), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_UINT32);
    ck_assert_uint_eq(((UA_UInt32 *)var.data)[2], 140);
    UA_Variant_clear(&var);
    //extension object with nested struct
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1005), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_SERVERSTATUSDATATYPE);
    ck_assert_int_eq(((UA_ServerStatusDataType *)var.data)->state, 5);
    UA_Variant_clear(&var);
    //array of extension object with nested struct
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1006), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_SERVERSTATUSDATATYPE);
    ck_assert_int_eq(((UA_ServerStatusDataType *)var.data)[1].state, 3);
    UA_Variant_clear(&var);
    // LocalizedText
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1007), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_LOCALIZEDTEXT);
    UA_String s = UA_STRING("en");
    ck_assert(UA_String_equal(&((UA_LocalizedText *)var.data)->locale, &s));
    UA_Variant_clear(&var);
    // LocalizedTextArray
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1008), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_LOCALIZEDTEXT);
    ck_assert(var.arrayLength==2);
    s = UA_STRING("griasEich!");
    ck_assert(UA_String_equal(&((UA_LocalizedText *)var.data)->text, &s));
    s = UA_STRING("Hi!");
    ck_assert(UA_String_equal(&((UA_LocalizedText*)var.data)[1].text, &s));
    UA_Variant_clear(&var);
    // QualifiedName
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1009), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_QUALIFIEDNAME);
    ck_assert_uint_eq(((UA_QualifiedName *)var.data)->namespaceIndex, 2);
    UA_Variant_clear(&var);
}
END_TEST

START_TEST(EnumValues)
{
    UA_Variant var;
    UA_StatusCode retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 1010), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_ENUMVALUETYPE);
    UA_EnumValueType* enumVal = (UA_EnumValueType*)var.data;
    ck_assert(enumVal[0].value==0);
    ck_assert(enumVal[1].value==1);
    ck_assert(!strncmp(enumVal[0].displayName.text.data, "LOG_ON", enumVal[0].displayName.text.length));
    ck_assert(!strncmp(enumVal[1].displayName.text.data, "LOG_OFF",
                       enumVal[1].displayName.text.length));
    UA_Variant_clear(&var);
}
END_TEST

START_TEST(NumericRange)
{
    UA_Variant var;
    UA_StatusCode retval =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 15007), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_STRING);
    UA_String *numericRange = (UA_String *)var.data;
    ck_assert(!strncmp(numericRange[0].data, "1:65535", numericRange[0].length));
    UA_Variant_clear(&var);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_LoadNS0Values);
    tcase_add_test(tc_server, EnumValues);
    tcase_add_test(tc_server, NumericRange);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char*argv[]) {
    printf("%s", argv[0]);
    if (!(argc > 1))
        return 1;
    nodesetPath = argv[1];
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
