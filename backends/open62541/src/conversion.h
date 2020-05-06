#ifndef CONVERSION_H
#define CONVERSION_H
#include <NodesetLoader/NodesetLoader.h>
#include <open62541/types.h>

static inline UA_Boolean isNodeId(const char *s)
{
    if (!s)
    {
        return UA_FALSE;
    }
    if (!strncmp(s, "ns=", 3) || !strncmp(s, "i=", 2) || !strncmp(s, "s=", 2) ||
        !strncmp(s, "g=", 2) || !strncmp(s, "b=", 2))
    {
        return UA_TRUE;
    }
    return UA_FALSE;
}

static inline UA_Boolean isTrue(const char *s)
{
    if (!s)
    {
        return UA_FALSE;
    }
    if (strcmp(s, "true"))
    {
        return UA_FALSE;
    }
    return UA_TRUE;
}

static inline UA_NodeId getNodeIdFromChars(TNodeId tid)
{
    UA_NodeId id = UA_NODEID_NULL;
    if (!tid.id)
    {
        return UA_NODEID_NULL;
    }
    UA_String idString;
    idString.length = strlen(tid.id);
    idString.data = (UA_Byte*)tid.id;
    UA_StatusCode result = UA_NodeId_parse(&id, idString);
    if (result != UA_STATUSCODE_GOOD)
    {
        return id;
    }
    id.namespaceIndex = (UA_UInt16)tid.nsIdx;
    return id;
}

static inline UA_NodeId extractNodeId(char *s)
{
    UA_NodeId id = UA_NODEID_NULL;
    if(!s)
    {
        return id;
    }    
    UA_String idString;
    idString.length=strlen(s);
    idString.data = (UA_Byte*)s;
    UA_StatusCode result = UA_NodeId_parse(&id, idString);
    if(result!=UA_STATUSCODE_GOOD)
    {
        return id;
    }
    return id;
}

#endif
