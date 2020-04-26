#ifndef __BACKEND_OPEN62541_DATATYPES_H__
#define __BACKEND_OPEN62541_DATATYPES_H__
#include <open62541/types.h>

struct UA_Server;
const struct UA_DataType *getCustomDataType(struct UA_Server *server,
                                            const UA_NodeId *typeId);

#endif