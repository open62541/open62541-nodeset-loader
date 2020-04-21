#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <openBackend.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, const char* argv[])
{
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    FileContext handler;
    handler.callback = Backend_addNode;
    handler.addNamespace = Backend_addNamespace;
    handler.userContext = server;
    ValueInterface valIf;
    valIf.userContext = NULL;
    valIf.newValue = Value_new;
    valIf.start = Value_start;
    valIf.end = Value_end;
    valIf.finish = Value_finish;
    valIf.deleteValue = Value_delete;
    handler.valueHandling = &valIf;

    for (int cnt = 1; cnt < argc; cnt++)
    {
        handler.file = argv[cnt];
        if (!loadFile(&handler))
        {
            printf("nodeset could not be loaded, exit\n");
            return 1;
        }
    }

    UA_Server_run(server, &running);

    UA_Server_delete(server);
}