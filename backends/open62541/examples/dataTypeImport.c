#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <openBackend.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, const char *argv[])
{
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    for (int cnt = 1; cnt < argc; cnt++)
    {
        if (!NodesetLoader_loadFile(server, argv[cnt], NULL))
        {
            printf("nodeset could not be loaded, exit\n");
            return 1;
        }
    }

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_NODEID_NUMERIC(2, 3002);
    UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(0, 85),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "myVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    // how to set the value of the variable with the loaded datatype?
    // at the moment via getCustomDataType
    struct Point
    {
        UA_Int32 x;
        UA_Int32 y;
    };

    struct Point p1 = {1, 2};

    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(
        &var, &p1, getCustomDataType(server, &attr.dataType));

        UA_findDataType(const UA_NodeId *typeId)

    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1000), var);

    UA_Server_run(server, &running);
    UA_Variant_clear(&var);
    UA_Server_delete(server);
}