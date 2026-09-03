#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <NodesetLoader/backendOpen62541.h>

#include <stdio.h>

static volatile UA_Boolean running = true;

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

    UA_Server_run(server, &running);
    UA_Server_delete(server);
}
