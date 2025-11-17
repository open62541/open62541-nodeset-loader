#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

struct Point
{
    UA_Int32 x;
    UA_Int32 y;
    UA_Int32 z;
};

void addPoint(UA_Server *server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_NODEID_NUMERIC(2, 3002);
    UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(0, 85),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "myVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    
    struct Point p1 = {1, 2, 3};

    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &p1, NodesetLoader_getCustomDataType(server, &attr.dataType));
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1000), var);
}

void addStructWithArray(UA_Server *server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_NODEID_NUMERIC(2, 3006);
    UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, 1001), UA_NODEID_NUMERIC(0, 85),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "myVarWithArray"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    struct StructWithArray
    {
        UA_Boolean valid;
        size_t size;
        UA_Int32 *data;
    };

    UA_Int32 data[3] = {1, 2, 3};
    struct StructWithArray s;
    s.data = data;
    s.valid = true;
    s.size = 3;

    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &s, NodesetLoader_getCustomDataType(server, &attr.dataType));

    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1001), var);
}

void addStructWithPointArray(UA_Server *server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_NODEID_NUMERIC(2, 3007);
    UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, 1002), UA_NODEID_NUMERIC(0, 85),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "myVarWithPointArray"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    struct StructWithPointArray
    {
        UA_Boolean valid;
        size_t size;
        struct Point *data;
    };

    struct Point pointData_1 = {1, 2, 3};
    struct Point pointData_2 = {11, 12, 13};
    struct Point pointData[2];
    pointData[0] = pointData_1;
    pointData[1] = pointData_2;

    struct StructWithPointArray structWithPointData;
    structWithPointData.data = pointData;
    structWithPointData.valid = true;
    structWithPointData.size = 2;

    UA_Variant var;
    UA_Variant_init(&var);

    UA_Variant_setScalar(&var, &structWithPointData,
                         NodesetLoader_getCustomDataType(server, &attr.dataType));

    UA_StatusCode retval =
        UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1002), var);
    assert(UA_STATUSCODE_GOOD == retval);
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


    addPoint(server);
    addStructWithArray(server);
    addStructWithPointArray(server);

    UA_Server_run(server, &running);
    UA_Server_delete(server);
}
