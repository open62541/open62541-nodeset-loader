/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"

#include "../testHelper.h"
#include "open62541/types_optionsetgen_generated.h"
#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

UA_Server *server;
char *nodesetPath = NULL;

static void setup(void)
{
    printf("path to testnodesets %s\n", nodesetPath);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void)
{

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(compareOptionSet)
{
    ck_assert(NodesetLoader_loadFile(server, nodesetPath, NULL));

    setNamespaceIndexOfGeneratedStruct(
        server, "http://yourorganisation.org/optionSet/",
        UA_TYPES_OPTIONSETGEN, UA_TYPES_OPTIONSETGEN_COUNT);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config->customDataTypes);

    ck_assert(config->customDataTypes->typesSize == UA_TYPES_OPTIONSETGEN_COUNT);

    for (const UA_DataType *generatedType = UA_TYPES_OPTIONSETGEN;
         generatedType !=
         UA_TYPES_OPTIONSETGEN + UA_TYPES_OPTIONSETGEN_COUNT;
         generatedType++)
    {
        const UA_DataType *importedType =
            NodesetLoader_getCustomDataType(server, &generatedType->typeId);
        ck_assert(importedType != NULL);
        typesAreMatching(generatedType, importedType, &UA_TYPES_OPTIONSETGEN[0],
                         config->customDataTypes->types);
    }
}
END_TEST

static Suite *testSuite_Client(void)
{
    Suite *s = suite_create("datatype Import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, compareOptionSet);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[])
{
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
