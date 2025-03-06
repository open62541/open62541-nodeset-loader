/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/types_generated.h>

#include "Value.h"
#include "conversion.h"
#include "NodesetLoader/NodesetLoader.h"
#include "nodeset_base64.h"
#include "ServerContext.h"

#include <assert.h>

typedef struct TypeList TypeList;
struct TypeList
{
    const UA_DataType *type;
    size_t memberIndex;
    TypeList *next;
};

typedef void (*ConversionFn)(uintptr_t adr, const char *value);

static void setBoolean(uintptr_t adr, const char *value)
{
    *(UA_Boolean *)adr = isValTrue(value);
}

static void setSByte(uintptr_t adr, const char *value)
{
    *(UA_SByte *)adr = (UA_SByte)atoi(value);
}

static void setByte(uintptr_t adr, const char *value)
{
    *(UA_Byte *)adr = (UA_Byte)atoi(value);
}

static void setInt16(uintptr_t adr, const char *value)
{
    *(UA_Int16 *)adr = (UA_Int16)atoi(value);
}

static void setUInt16(uintptr_t adr, const char *value)
{
    *(UA_UInt16 *)adr = (UA_UInt16)atoi(value);
}

static void setInt32(uintptr_t adr, const char *value)
{
    *(UA_Int32 *)adr = (UA_Int32)atol(value);
}

static void setUInt32(uintptr_t adr, const char *value)
{
    *(UA_UInt32 *)adr = (UA_UInt32)atol(value);
}

static void setInt64(uintptr_t adr, const char *value)
{
    *(UA_Int64 *)adr = (UA_Int64)atoll(value);
}

static void setUInt64(uintptr_t adr, const char *value)
{
    *(UA_UInt64 *)adr = (UA_UInt64)atoll(value);
}

static void setFloat(uintptr_t adr, const char *value)
{
    *(UA_Float *)adr = (UA_Float)atof(value);
}

static void setDouble(uintptr_t adr, const char *value)
{
    *(UA_Double *)adr = atof(value);
}

static void setString(uintptr_t adr, const char *value)
{
    UA_String *s = (UA_String *)adr;

    
    s->length = strlen(value);
    s->data = (UA_Byte*)calloc(s->length, 1);
    memcpy(s->data, value, s->length);
    //s->data = (UA_Byte *)(uintptr_t)value;
}

#define CONVERSION_TABLE_SIZE 12
static const ConversionFn conversionTable[CONVERSION_TABLE_SIZE] =
    {setBoolean, setSByte, setByte,   setInt16, setUInt16, setInt32,
     setUInt32,  setInt64, setUInt64, setFloat, setDouble, setString};

static void
setScalarValueWithAddress(uintptr_t adr, UA_UInt32 kind, const char *value)
{
    if (value)
    {
        conversionTable[kind](adr, value);
    }
}

RawData *RawData_new(RawData *old)
{
    RawData *data = (RawData *)calloc(1, sizeof(RawData));
    if (!data)
    {
        return NULL;
    }
    if(old)
    {
        while (old->next)
        {
            old = old->next;
        }
        old->next = data;
    }

    return data;
}

void RawData_delete(RawData *data)
{
    while(data)
    {
        RawData* tmp = data;
        data=data->next;
        //free(tmp->mem);
        free(tmp);
    }
}

static void setPrimitiveValue(RawData *data, const char *value,
                              UA_DataTypeKind kind, size_t memSize)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;
    setScalarValueWithAddress(adr, kind, value);
    data->offset = data->offset + memSize;
}

static NL_Data *lookupMember(const NL_Data *value, const char *name)
{
    // is there a better way?
    for (size_t cnt = 0; cnt < value->val.complexData.membersSize; cnt++)
    {
        if (!strcmp(value->val.complexData.members[cnt]->name, name))
        {
            return value->val.complexData.members[cnt];
        }
    }
    return NULL;
}

static void setDateTime(const NL_Data *value, RawData *data)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;
    UA_DateTime time = UA_DateTime_fromString(value->val.primitiveData.value);
    *(UA_DateTime*)adr = time;
    data->offset = data->offset + sizeof(UA_DateTime);
}

static void setQualifiedName(const NL_Data *value, RawData *data, const ServerContext *serverContext)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;

    setScalarValueWithAddress(
        (uintptr_t)&((UA_QualifiedName*)adr)->namespaceIndex,
        UA_DATATYPEKIND_UINT16,
        value->val.complexData.members[0]->val.primitiveData.value);
    
    // Translate namespace index
    ((UA_QualifiedName*)adr)->namespaceIndex = 
        ServerContext_translateToServerIdx(serverContext, ((UA_QualifiedName*)adr)->namespaceIndex);        

    setScalarValueWithAddress(
        (uintptr_t)&((UA_QualifiedName*)adr)->name,
        UA_DATATYPEKIND_STRING,
        value->val.complexData.members[1]->val.primitiveData.value);

    data->offset += sizeof(UA_QualifiedName);
}

static void setLocalizedText(const NL_Data *value, RawData *data)
{
    NL_Data *localeData = lookupMember(value, "Locale");

    if (localeData)
    {
        setScalarValueWithAddress(data->offset + (uintptr_t) &
                                      ((UA_LocalizedText *)data->mem)->locale,
                                  UA_DATATYPEKIND_STRING,
                                  localeData->val.primitiveData.value);
    }

    NL_Data *textData = lookupMember(value, "Text");

    if (textData)
    {
        setScalarValueWithAddress(
            data->offset + (uintptr_t) & ((UA_LocalizedText *)data->mem)->text,
            UA_DATATYPEKIND_STRING, textData->val.primitiveData.value);
    }
    data->offset += sizeof(UA_LocalizedText);
}

static void setNodeId(const NL_Data *value, RawData *data, const ServerContext *serverContext)
{
    assert(value->val.complexData.membersSize == 1);
    *(UA_NodeId *)((uintptr_t)data->mem + data->offset) =
        extractNodeId((char *)(uintptr_t)value->val.complexData.members[0]
                          ->val.primitiveData.value);

    // Translate namespace index
    (*(UA_NodeId *)((uintptr_t)data->mem + data->offset)).namespaceIndex =
        ServerContext_translateToServerIdx(serverContext, (*(UA_NodeId *)((uintptr_t)data->mem + data->offset)).namespaceIndex);

    data->offset += sizeof(UA_NodeId);
}

static void setByteString(const NL_Data* value, RawData*data)
{
    UA_ByteString *s = (UA_ByteString *)((uintptr_t)data->mem+data->offset);
    int len = 0;
    unsigned char *val = NULL;

    if (value->val.primitiveData.value)
    {
        val = unbase64(value->val.primitiveData.value, (int)strlen(value->val.primitiveData.value), &len);
    }

    s->length = (size_t)len;
    s->data = val;
}

static void setGuid(const NL_Data* value, RawData*data)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;
    *(UA_Guid*)adr = UA_GUID((const char *)(value->val.complexData.members[0]->val.primitiveData.value));
    data->offset = data->offset + sizeof(UA_Guid);
}

static void setScalar(const NL_Data *value, const UA_DataType *type, RawData *data,
                      const UA_DataType *customTypes, const ServerContext *serverContext);
static void setArray(const NL_Data *value, const UA_DataType *type, RawData *data,
                     const UA_DataType *customTypes, const ServerContext *serverContext);

static const char* getEnumValuePos(const char *string)
{
    // Enum value is either on <symbol>_<value> or <value> format
    char *enumSepPos = strchr(string, '_');
    if (enumSepPos)
    {
        return ++enumSepPos;
    }
    return string;
}

static void setStructure(const NL_Data *value, const UA_DataType *type,
                         RawData *data, const UA_DataType *customTypes,
                         const ServerContext *serverContext)
{
    assert(value->type == DATATYPE_COMPLEX);
    // there can be less members specified then the type requires
    for (const UA_DataTypeMember *m = type->members;
         m != type->members + type->membersSize; m++)
    {
        const UA_DataType* memberType = m->memberType;

        data->offset += m->padding;
        NL_Data *memberData = lookupMember(value, m->memberName);
        if (!memberData)
        {
            data->offset += memberType->memSize;
            return;
        }
        if (m->isArray)
        {
            RawData *rawdata = RawData_new(data);
            rawdata->mem = calloc(memberData->val.complexData.membersSize,
                                  memberType->memSize);
            setArray(memberData, memberType, rawdata, customTypes, serverContext);
            size_t *size = (size_t *)((uintptr_t)data->mem + data->offset);
            *size = memberData->val.complexData.membersSize;
            data->offset += sizeof(size_t);
            void **d = (void **)((uintptr_t)data->mem + data->offset);
            *d = rawdata->mem;
            data->offset += sizeof(void *);
        }
        else
        {
            size_t structOffset = data->offset + memberType->memSize;
            setScalar(memberData, memberType, data, customTypes, serverContext);
            data->offset = structOffset;
        }
    }
}

static void setUnion(const NL_Data *value, const UA_DataType *type, RawData *data,
                        const UA_DataType *customTypes, const ServerContext *serverContext)
{
    // For a reference, decoded union has the following layout in code:
    //
    // struct UnionType
    // {                             <-|
    //     UA_UInt32 switchField;      |-fields padding (from the begginning of the struct)
    //                               <-|
    //     union {
    //         TYPE_1 field_1;
    //         TYPE_2 field_2;
    //         ...
    //     } fields;
    //                               <-- end padding
    // }

    // Remember the initial offset of the complete union
    const size_t unionOffset = data->offset;
    // Extract the switchField value
    const UA_UInt32 switchField = (UA_UInt32)atoi(value->val.complexData.members[0]->val.primitiveData.value);
    // Write out the switchField value
    *(UA_UInt32 *)((uintptr_t)data->mem + data->offset) = switchField;
    // Insert the field padding

    // If the switch field is 0 then no field is present.
    // A Union with no fields present has the same meaning as a NULL value.
    // See: https://reference.opcfoundation.org/v104/Core/docs/Part6/5.2.8/
    if (switchField != 0) {
        data->offset += type->members[switchField - 1].padding;
        // For convenience setup direct pointers to data and type
        NL_Data *memberData = value->val.complexData.members[1];
        const UA_DataType* memberType = type->members[switchField - 1].memberType;

        // Write out the field value
        if (type->members[switchField - 1].isArray)
        {
            RawData *rawdata = RawData_new(data);
            rawdata->mem = calloc(memberData->val.complexData.membersSize,
                                    memberType->memSize);
            setArray(memberData, memberType, rawdata, customTypes, serverContext);
            size_t *size = (size_t *)((uintptr_t)data->mem + data->offset);
            *size = memberData->val.complexData.membersSize;
            data->offset += sizeof(size_t);
            void **d = (void **)((uintptr_t)data->mem + data->offset);
            *d = rawdata->mem;
            data->offset += sizeof(void *);
        }
        else
        {
            setScalar(memberData, memberType, data, customTypes, serverContext);
        }
        // Overwrite the offset using the size of the complete union, which includes also "end padding" of a C structure.
        data->offset = unionOffset + type->memSize;
    }
}

static void setStatusCode(const NL_Data *value, RawData *data)
{
    // StatusCode is a complex type with a primitive
    assert(value->type == DATATYPE_COMPLEX);
    if (value->val.complexData.membersSize == 1)
    {
        NL_Data *code = value->val.complexData.members[0];
        setPrimitiveValue(
            data, code->val.primitiveData.value, UA_DATATYPEKIND_UINT32,
            UA_TYPES[UA_TYPES_UINT32].memSize);
    }
}

static void setScalar(const NL_Data *value, const UA_DataType *type, RawData *data,
                      const UA_DataType *customTypes, const ServerContext *serverContext)
{
    if (type->typeKind < CONVERSION_TABLE_SIZE)
    {
        setPrimitiveValue(data, value->val.primitiveData.value,
                          (UA_DataTypeKind)type->typeKind, type->memSize);
    }
    else if(type->typeKind == UA_DATATYPEKIND_BYTESTRING)
    {
        setByteString(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_NODEID)
    {
        setNodeId(value, data, serverContext);
    }
    else if (type->typeKind == UA_DATATYPEKIND_QUALIFIEDNAME)
    {
        setQualifiedName(value, data, serverContext);
    }
    else if (type->typeKind == UA_DATATYPEKIND_LOCALIZEDTEXT)
    {
        setLocalizedText(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_DATETIME)
    {
        setDateTime(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_ENUM)
    {
        setPrimitiveValue(data, getEnumValuePos(value->val.primitiveData.value),
                          UA_DATATYPEKIND_INT32,
                          UA_TYPES[UA_TYPES_INT32].memSize);
    }
    else if (type->typeKind == UA_DATATYPEKIND_STRUCTURE)
    {
        setStructure(value, type, data, customTypes, serverContext);
    }
    else if (type->typeKind == UA_DATATYPEKIND_GUID)
    {
        setGuid(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_UNION)
    {
        setUnion(value, type, data, customTypes, serverContext);
    }
    else if (type->typeKind == UA_DATATYPEKIND_STATUSCODE)
    {
        setStatusCode(value, data);
    }
    else
    {
        assert(false && "conversion not implemented");
    }
}

static void setArray(const NL_Data *value, const UA_DataType *type, RawData *data,
                     const UA_DataType *customTypes, const ServerContext *serverContext)
{
    for (size_t i = 0; i < value->val.complexData.membersSize; i++)
    {
        data->offset = i * type->memSize;
        setScalar(value->val.complexData.members[i], type, data, customTypes, serverContext);
    }
}

void Value_getData(RawData *outData, const NL_Value *value, const UA_DataType *type,
                   const UA_DataType *customTypes, const ServerContext *serverContext)
{
    if (!type)
    {
        return;
    }

    if (value->isArray)
    {
        if (value->data->val.complexData.membersSize == 0)
        {
            outData = NULL;
        }
        else
        {
            outData->mem =
                calloc(value->data->val.complexData.membersSize, type->memSize);
            setArray(value->data, type, outData, customTypes, serverContext);
        }
    }
    else
    {
        outData->mem = calloc(1, type->memSize);
        setScalar(value->data, type, outData, customTypes, serverContext);
    }
}
