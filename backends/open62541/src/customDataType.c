#include <NodesetLoader/dataTypes.h>
#include <open62541/server_config.h>

const struct UA_DataType *getCustomDataType(struct UA_Server *server,
                                            const UA_NodeId *typeId)
{
    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_DataTypeArray *types = config->customDataTypes;
    while (types)
    {
        const UA_DataTypeArray *next = types->next;
        if (types->types)
        {
            for (const UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                if (UA_NodeId_equal(&type->typeId, typeId))
                {
                    return type;
                }
            }
        }
        types = next;
    }
    return NULL;
}
