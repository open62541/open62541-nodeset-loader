#ifndef __BACKEND_OPEN62541_DATATYPES_H__
#define __BACKEND_OPEN62541_DATATYPES_H__
#include <open62541/types.h>

#if __GNUC__ || __clang__
#define LOADER_EXPORT __attribute__((visibility("default")))
#endif
#ifndef LOADER_EXPORT
#define LOADER_EXPORT /* fallback to default */
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct UA_Server;
LOADER_EXPORT const struct UA_DataType *
getCustomDataType(struct UA_Server *server, const UA_NodeId *typeId);

#ifdef __cplusplus
}
#endif

#endif
