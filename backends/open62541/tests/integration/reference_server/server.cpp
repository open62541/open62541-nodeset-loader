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

UA_Boolean AddDataTypeArray(UA_DataTypeArray **ppDataTypeArray,
                            UA_UInt32 &ioDataTypeArraySize,
                            const UA_DataType *pNewDataTypes,
                            const size_t NewDataTypesSize)
{
    if ((ppDataTypeArray == 0) || (pNewDataTypes == 0))
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Error AddDataTypeArray(): null pointer param");
        return UA_FALSE;
    }
    // allocate additional memory for DataTypeArray
    *ppDataTypeArray = (UA_DataTypeArray *)UA_realloc(
        (void *)*ppDataTypeArray,
        (ioDataTypeArraySize + 1) * sizeof(UA_DataTypeArray));
    if (*ppDataTypeArray == 0)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Error AddDataTypeArray(): Out of memory");
        return UA_FALSE;
    }
    (*ppDataTypeArray)[ioDataTypeArraySize].next = 0;
    (*ppDataTypeArray)[ioDataTypeArraySize].types = pNewDataTypes;
    /* TODO: why has UA_DataTypeArray struct definition const qualifiers?
    it makes an allocation on heap cumbersome ...
    or is there any better way than casting const away? */
    *(const_cast<size_t *>(&(
        (*ppDataTypeArray)[ioDataTypeArraySize].typesSize))) = NewDataTypesSize;

    // if there was a previous DataTypeArray, we have to make the link to the
    // new node
    if (ioDataTypeArraySize > 0)
    {
        ppDataTypeArray[ioDataTypeArraySize - 1]->next =
            &(*ppDataTypeArray)[ioDataTypeArraySize];
    }
    ioDataTypeArraySize++;
    return UA_TRUE;
}

void FreeDataTypeArray(UA_DataTypeArray *pDataTypeArray)
{
    if (pDataTypeArray != 0)
    {
        UA_free(pDataTypeArray);
        pDataTypeArray = 0;
    }
}

#ifdef USE_DI
// TODO: write generic function and test other struct/enum variables
UA_Boolean AddDIStructVariables(UA_Server *pServer)
{
    if (pServer == 0)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Error AddDIStructVariables(): null pointer");
        return UA_FALSE;
    }

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
#endif

int main()
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *pCfg = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(pCfg);

    // prepare custom datatype arrays
    UA_DataTypeArray *pDataTypeArray = 0;
    UA_UInt32 NoOfDataTypeArrays = 0;

    /* create nodes from nodeset */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

#ifdef USE_DI
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Adding the DI namespace.");
    retval = namespace_integration_test_di_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the DI namespace failed. Please check previous "
                     "error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    if (AddDataTypeArray(&pDataTypeArray, NoOfDataTypeArrays,
                         UA_TYPES_INTEGRATION_TEST_DI,
                         UA_TYPES_INTEGRATION_TEST_DI_COUNT) == UA_FALSE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the DI data types failed.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    if (AddDIStructVariables(server) == UA_FALSE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding DI structure variables failed.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

#endif

#ifdef USE_PLC_OPEN
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Adding the PLCopen namespace.");
    retval |= namespace_integration_test_plc_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the PLCopen namespace failed. Please check "
                     "previous error output.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    // PLCopen does not define types
#endif

#ifdef USE_EUROMAP_83
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Adding the Euromap 83 namespace.");
    retval |= namespace_integration_test_euromap_83_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap83 namespace failed. Please check "
                     "previous error output.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    if (AddDataTypeArray(&pDataTypeArray, NoOfDataTypeArrays,
                         UA_TYPES_INTEGRATION_TEST_EUROMAP_83,
                         UA_TYPES_INTEGRATION_TEST_EUROMAP_83_COUNT) ==
        UA_FALSE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap83 data types failed.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
#endif

#ifdef USE_EUROMAP_77
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Adding the Euromap 77 namespace.");
    retval |= namespace_integration_test_euromap_77_generated(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap77 namespace failed. Please check "
                     "previous error output.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    if (AddDataTypeArray(&pDataTypeArray, NoOfDataTypeArrays,
                         UA_TYPES_INTEGRATION_TEST_EUROMAP_77,
                         UA_TYPES_INTEGRATION_TEST_EUROMAP_77_COUNT) ==
        UA_FALSE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Euromap77 data types failed.");
        FreeDataTypeArray(pDataTypeArray);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
#endif

    pCfg->customDataTypes = pDataTypeArray;

    retval = UA_Server_run(server, &running);

    UA_free(pDataTypeArray);
    pDataTypeArray = 0;

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
