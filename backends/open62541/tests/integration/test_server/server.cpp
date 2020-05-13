#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <NodesetLoader/backendOpen62541.h>

#include <signal.h>
#include <stdlib.h>

using namespace std;

UA_Boolean running = true;

static void stopHandler(int sign)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
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
            return EXIT_FAILURE;
        }
    }

    UA_StatusCode retval = UA_Server_run(server, &running);

    // print the custom dataTypes
    const UA_DataTypeArray *customTypesArray = pConfig->customDataTypes;
    while (customTypesArray)
    {
        printf("typesStart: %p typesEnd: %p typesSize: %zu\n",
               customTypesArray->types,
               customTypesArray->types + customTypesArray->typesSize,
               customTypesArray->typesSize);
        for (const UA_DataType *type = customTypesArray->types;
             type != customTypesArray->types + customTypesArray->typesSize;
             type++)
        {
            UA_String s;
            UA_String_init(&s);
            UA_NodeId_print(&type->typeId, &s);
            printf("type: %p typeId: %.*s ", type, (int)s.length, s.data);
            printf("typeIndex: %d \n", type->typeIndex);
        }
        customTypesArray = customTypesArray->next;
    }

    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
