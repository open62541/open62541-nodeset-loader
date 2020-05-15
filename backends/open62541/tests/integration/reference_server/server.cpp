#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include <signal.h>
#include <stdlib.h>

#ifdef USE_DI
#include "open62541/namespace_integration_test_di_generated.h"
#endif

#ifdef USE_PLC_OPEN
#include "open62541/namespace_integration_test_plc_generated.h"
#endif

#ifdef USE_EUROMAP_83
#include "open62541/namespace_integration_test_euromap_83_generated.h"
#endif

#ifdef USE_EUROMAP_77
#include "open62541/namespace_integration_test_euromap_77_generated.h"
#endif

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
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

#ifdef USE_DI
    retval = namespace_integration_test_di_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the DI namespace failed. Please check previous "
                     "error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
#endif

#ifdef USE_PLC_OPEN
    retval |= namespace_integration_test_plc_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the PLCopen namespace failed. Please check "
                     "previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
#endif

#ifdef USE_EUROMAP_83
    retval |= namespace_integration_test_euromap_83_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap83 namespace failed. Please check "
                     "previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

#endif

#ifdef USE_EUROMAP_77
    retval |= namespace_integration_test_euromap_77_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap77 namespace failed. Please check "
                     "previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
#endif

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
