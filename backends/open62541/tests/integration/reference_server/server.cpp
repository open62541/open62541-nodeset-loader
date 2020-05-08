#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <cassert>
#include <signal.h>
#include <stdlib.h>

#include "open62541/namespace_integration_test_di_generated.h"
#include "open62541/namespace_integration_test_plc_generated.h"
// #include "open62541/namespace_integration_test_euromap_77_generated.h"
// #include "open62541/namespace_integration_test_euromap_83_generated.h"

using namespace std;

UA_Boolean running = true;

static void stopHandler(int sign)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main()
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /* create nodes from nodeset */
    UA_StatusCode retval = namespace_integration_test_di_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the DI namespace failed. Please check previous "
                     "error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    retval |= namespace_integration_test_plc_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the PLCopen namespace failed. Please check "
                     "previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    // retval |= namespace_integration_test_euromap_83_generated(server);
    // if (retval != UA_STATUSCODE_GOOD)
    // {
    //     UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
    //                  "Adding the Euromap83 namespace failed. Please check "
    //                  "previous error output.");
    //     UA_Server_delete(server);
    //     return EXIT_FAILURE;
    // }
    // retval |= namespace_integration_test_euromap_77_generated(server);
    // if (retval != UA_STATUSCODE_GOOD)
    // {
    //     UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
    //                  "Adding the Euromap77 namespace failed. Please check "
    //                  "previous error output.");
    //     UA_Server_delete(server);
    //     return EXIT_FAILURE;
    // }

    UA_TransferResultDataDataType TransferResultDataData;
    TransferResultDataData.endOfResults = UA_FALSE;
    TransferResultDataData.parameterDefsSize = 0;
    TransferResultDataData.parameterDefs = 0;
    TransferResultDataData.sequenceNumber = 0;

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.displayName =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.dataType =
        UA_TYPES_INTEGRATION_TEST_DI
            [UA_TYPES_INTEGRATION_TEST_DI_TRANSFERRESULTDATADATATYPE]
                .typeId;
    UA_Variant_setScalar(
        &vattr.value, &TransferResultDataData,
        &UA_TYPES_INTEGRATION_TEST_DI
            [UA_TYPES_INTEGRATION_TEST_DI_TRANSFERRESULTDATADATATYPE]);

    assert(UA_Server_addVariableNode(
               server, UA_NODEID_STRING(1, (char *)"TransferResultDataData"),
               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
               UA_QUALIFIEDNAME(1, (char *)"TransferResultDataData"),
               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL,
               NULL) == UA_STATUSCODE_GOOD);

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
