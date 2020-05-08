#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>

#include <cassert>
#include <iostream>
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

    // TODO: test structure instantiation
    UA_NodeId TransferResultDataDataTypeId = UA_NODEID_NUMERIC(2, 15889);
    const UA_DataType *importedType =
        getCustomDataType(server, &TransferResultDataDataTypeId);

    assert(importedType != 0);

    assert(importedType->membersSize == 3);

    UA_Byte *pTransferResultDataDataStruct =
        (UA_Byte *)UA_calloc(importedType->memSize, 1);
    assert(pTransferResultDataDataStruct != 0);
    // check member types and sizes etc. and set structure fields accordingly

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.displayName =
        UA_LOCALIZEDTEXT((char *)"en-US", (char *)"TransferResultDataData");
    vattr.dataType = importedType->typeId;
    UA_Variant_setScalar(&vattr.value, pTransferResultDataDataStruct,
                         importedType);

    assert(UA_Server_addVariableNode(
               server, UA_NODEID_STRING(1, (char *)"TransferResultDataData"),
               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
               UA_QUALIFIEDNAME(1, (char *)"TransferResultDataData"),
               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL,
               NULL) == UA_STATUSCODE_GOOD);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);

    UA_free(pTransferResultDataDataStruct);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
