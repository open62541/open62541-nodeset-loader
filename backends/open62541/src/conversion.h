/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef CONVERSION_H
#define CONVERSION_H
#include <NodesetLoader/NodesetLoader.h>
#include <open62541/types.h>
#include <time.h>

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

static inline UA_DateTime UA_DateTime_fromString(const char *dateString)
{
    UA_DateTimeStruct dt;
    memset(&dt, 0, sizeof(UA_DateTimeStruct));
    sscanf(dateString, "%hi-%hu-%huT%hu:%hu:%huZ",
           &dt.year, &dt.month, &dt.day, &dt.hour, &dt.min, &dt.sec);
    UA_DateTime dateTime = UA_DateTime_fromStruct(dt);
    return dateTime;
}

#endif
