#ifndef CONVERSION_H
#define CONVERSION_H
#include <nodesetLoader/NodesetLoader.h>
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

static inline UA_NodeId getNodeIdFromChars(TNodeId id)
{
    if (!id.id)
    {
        return UA_NODEID_NULL;
    }
    UA_UInt16 nsidx = (UA_UInt16)id.nsIdx;

    switch (id.id[0])
    {
    // integer
    case 'i':
    {
        UA_UInt32 nodeId = (UA_UInt32)atoi(&id.id[2]);
        return UA_NODEID_NUMERIC(nsidx, nodeId);
        break;
    }
    case 's':
    {
        return UA_NODEID_STRING_ALLOC(nsidx, &id.id[2]);
        break;
    }
    }
    return UA_NODEID_NULL;
}

// todo: handle guid, bytestring
static inline UA_NodeId extractNodeId(char *s)
{
    if (!s || strlen(s) < 3)
    {
        return UA_NODEID_NULL;
    }
    char *idxSemi = strchr(s, ';');
    // namespaceindex zero?
    if (idxSemi == NULL)
    {
        switch (s[0])
        {
        // integer
        case 'i':
        {
            UA_UInt32 nodeId = (UA_UInt32)atoi(&s[2]);
            return UA_NODEID_NUMERIC(0, nodeId);
        }
        case 's':
        {
            return UA_NODEID_STRING_ALLOC(0, &s[2]);
        }
        }
    }
    else
    {
        UA_NodeId id;
        id.namespaceIndex = 0;
        UA_UInt16 nsIdx = (UA_UInt16)atoi(&s[3]);
        switch (idxSemi[1])
        {
        // integer
        case 'i':
        {
            UA_UInt32 nodeId = (UA_UInt32)atoi(&idxSemi[3]);
            id.namespaceIndex = nsIdx;
            id.identifierType = UA_NODEIDTYPE_NUMERIC;
            id.identifier.numeric = nodeId;
            break;
        }
        case 's':
        {
            UA_String sid = UA_STRING_ALLOC(&idxSemi[3]);
            id.namespaceIndex = nsIdx;
            id.identifierType = UA_NODEIDTYPE_STRING;
            id.identifier.string = sid;
            break;
        }
        default:
            return UA_NODEID_NULL;
        }
        return id;
    }
    return UA_NODEID_NULL;
}

#endif
