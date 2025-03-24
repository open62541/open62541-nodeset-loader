/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/server.h>

#include <NodesetLoader/dataTypes.h>
#include "customDataType.h"

const struct UA_DataType *findCustomDataType(const UA_NodeId *typeId,
                                       const UA_DataTypeArray *types)
{
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

#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
static void cleanupCustomTypes(const UA_DataTypeArray *types)
{
    while (types)
    {
        const UA_DataTypeArray *next = types->next;
        if (types->types)
        {
            for (const UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                free((void*)(uintptr_t)type->typeName);
                for (UA_DataTypeMember *m = type->members;
                     m != type->members + type->membersSize; m++)
                {
                    free((void*)(uintptr_t)m->memberName);
                    m->memberName = NULL;
                }
                free((void*)type->members);
            }
        }
        free((void*)(uintptr_t)types->types);
        free((void*)(uintptr_t)types);
        types = next;
    }
}
#endif

const struct UA_DataType *
NodesetLoader_getCustomDataType(struct UA_Server *server,
                                const UA_NodeId *typeId)
{
    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_DataTypeArray *types = config->customDataTypes;
    return findCustomDataType(typeId, types);
}

#ifdef NODESETLOADER_CLEANUP_CUSTOM_DATATYPES
void
NodesetLoader_cleanupCustomDataTypes(const UA_DataTypeArray *customTypes)
{
    cleanupCustomTypes(customTypes);
}
#endif
