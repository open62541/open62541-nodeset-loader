#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

#include <signal.h>
#include <stdlib.h>

using namespace std;

UA_Boolean running = true;

static void stopHandler(int sign)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

// TODO: write generic function and test other struct/enum variables
UA_Boolean AddDIStructVariables(UA_Server *pServer)
{
    if (pServer == 0)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Error AddDIStructVariables(): null pointer");
        return UA_FALSE;
    }

    // we can't know if the DI nodeset has been loaded
    // so if we can't find the type we assume that it's okay
    UA_NodeId TransferResultDataDataTypeId = UA_NODEID_NUMERIC(2, 15889);
    const UA_DataType *importedType =
        NodesetLoader_getCustomDataType(pServer, &TransferResultDataDataTypeId);
    if (importedType == 0)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "Info AddDIStructVariables(): could not find node Id: "
                    "ns=2;i=15889");
        return UA_TRUE;
    }

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.displayName =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.dataType = importedType->typeId;
    // we could set the value of TransferResultData if we would generically
    // parse the DataType defintion ...

    if (UA_Server_addVariableNode(
            pServer, UA_NODEID_STRING(1, (char *)"TransferResultDataData"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, (char *)"TransferResultDataData"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL,
            NULL) != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Error AddDIStructVariables(): memory allocation failed");
        return UA_FALSE;
    }
    return UA_TRUE;
}

int main(int argc, const char *argv[])
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *pConfig = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(pConfig, 4841, 0); // TODO: change this

    for (int cnt = 1; cnt < argc; cnt++)
    {
        printf("Load file: '%s'\n", argv[cnt]);
        if (!NodesetLoader_loadFile(server, argv[cnt], NULL))
        {
            printf("nodeset could not be loaded, exit\n");
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
            const UA_DataTypeArray *customTypes =
                UA_Server_getConfig(server)->customDataTypes;
#endif
            UA_Server_delete(server);
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
            NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif
            return EXIT_FAILURE;
        }
    }

    if (AddDIStructVariables(server) == UA_FALSE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding DI structure variables failed.");
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
        const UA_DataTypeArray *customTypes =
            UA_Server_getConfig(server)->customDataTypes;
#endif
        UA_Server_delete(server);
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
        NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif
        return EXIT_FAILURE;
    }

    UA_StatusCode retval = UA_Server_run(server, &running);
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
    const UA_DataTypeArray *customTypes =
        UA_Server_getConfig(server)->customDataTypes;
#endif
    UA_Server_delete(server);
#ifdef USE_CLEANUP_CUSTOM_DATATYPES
    NodesetLoader_cleanupCustomDataTypes(customTypes);
#endif

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
