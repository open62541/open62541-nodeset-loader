#include "Value.h"
#include "conversion.h"
#include <NodesetLoader/NodesetLoader.h>
#include <assert.h>
#include <open62541/types_generated.h>

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
    *(UA_Boolean *)adr = isTrue(value);
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
    *(UA_Int32 *)adr = atoi(value);
}

static void setUInt32(uintptr_t adr, const char *value)
{
    *(UA_UInt32 *)adr = (UA_UInt32)atoi(value);
}

static void setInt64(uintptr_t adr, const char *value)
{
    *(UA_Int64 *)adr = atoi(value);
}

static void setUInt64(uintptr_t adr, const char *value)
{
    *(UA_UInt64 *)adr = (UA_UInt64)atoi(value);
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
    s->data = (UA_Byte *)(uintptr_t)value;
}

static void setDateTime(uintptr_t adr, const char *value)
{
    printf("DateTime: %s currently not handled\n", value);
}

static void setNotImplemented(uintptr_t adr, const char *value)
{
    UA_assert(false && "not implemented");
}

static const ConversionFn conversionTable[UA_DATATYPEKINDS] = {
    setBoolean,        setSByte,          setByte,           setInt16,
    setUInt16,         setInt32,          setUInt32,         setInt64,
    setUInt64,         setFloat,          setDouble,         setString,
    setDateTime,       setNotImplemented,
    setString, // handle bytestring like string
    setString, // handle xmlElement like string
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented,
    setNotImplemented, // special handling needed
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented,
    setNotImplemented,
    setInt32, // handle enum the same way like int32
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented};

static void setScalarValueWithAddress(uintptr_t adr, UA_UInt32 kind,
                                      const char *value)
{
    if (value)
    {
        conversionTable[kind](adr, value);
    }
}

static RawData *RawData_new(void)
{
    RawData *data = (RawData *)calloc(1, sizeof(RawData));
    assert(data);
    return data;
}

void RawData_delete(RawData *data)
{
    if (data)
    {
        free(data->mem);
        free(data);
    }
}

static void setPrimitiveValue(RawData *data, const char *value,
                              UA_DataTypeKind kind, size_t memSize)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;
    setScalarValueWithAddress(adr, kind, value);
    data->offset = data->offset + memSize;
}

static Data *lookupMember(const Data *value, const char *name)
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

static void setQualifiedName(const Data *value, RawData *data)
{

    setScalarValueWithAddress(
        data->offset + (uintptr_t) &
            ((UA_QualifiedName *)data->mem)->namespaceIndex,
        UA_DATATYPEKIND_UINT16,
        value->val.complexData.members[0]->val.primitiveData.value);

    setScalarValueWithAddress(
        data->offset + (uintptr_t) & ((UA_QualifiedName *)data->mem)->name,
        UA_DATATYPEKIND_STRING,
        value->val.complexData.members[1]->val.primitiveData.value);

    data->offset += sizeof(UA_QualifiedName);
}

static void setLocalizedText(const Data *value, RawData *data)
{
    Data *localeData = lookupMember(value, "Locale");

    if (localeData)
    {
        setScalarValueWithAddress(data->offset + (uintptr_t) &
                                      ((UA_LocalizedText *)data->mem)->locale,
                                  UA_DATATYPEKIND_STRING,
                                  localeData->val.primitiveData.value);
    }

    Data *textData = lookupMember(value, "Text");

    if (textData)
    {
        setScalarValueWithAddress(
            data->offset + (uintptr_t) & ((UA_LocalizedText *)data->mem)->text,
            UA_DATATYPEKIND_STRING, textData->val.primitiveData.value);
    }
    data->offset += sizeof(UA_LocalizedText);
}

static void setNodeId(const Data *value, RawData *data)
{
    // TODO: translate namespaceIndex?
    assert(value->val.complexData.membersSize == 1);
    *(UA_NodeId *)((uintptr_t)data->mem + data->offset) =
        extractNodeId((char *)(uintptr_t)value->val.complexData.members[0]
                          ->val.primitiveData.value);
    data->offset += sizeof(UA_NodeId);
}

static void setScalar(const Data *value, const UA_DataType *type,
                      RawData *data, const UA_DataType* customTypes);

static void setStructure(const Data *value, const UA_DataType *type,
                         RawData *data, const UA_DataType* customTypes)
{
    assert(value->type == DATATYPE_COMPLEX);
    // there can be less members specified then the type requires
    // assert(value->val.complexData.membersSize == type->membersSize);
    for (const UA_DataTypeMember *m = type->members;
         m != type->members + type->membersSize; m++)
    {
        const UA_DataType *memberType = NULL;
        if (m->namespaceZero)
        {
            memberType = &UA_TYPES[m->memberTypeIndex];
        }
        else
        {
            memberType = customTypes+m->memberTypeIndex;
        }

        data->offset += m->padding;
        Data *memberData = lookupMember(value, m->memberName);
        if (m->isArray)
        {
            // TODO: arrays not implemented in frontend
            data->offset += sizeof(size_t);
            data->offset += sizeof(void *);
        }
        else
        {
            if (memberData)
            {
                setScalar(memberData, memberType, data, customTypes);
            }
            else
            {
                data->offset += memberType->memSize;
            }
        }
    }
}

static void setScalar(const Data *value, const UA_DataType *type, RawData *data, const UA_DataType* customTypes)
{
    if (type->typeKind <= UA_DATATYPEKIND_STRING)
    {
        setPrimitiveValue(data, value->val.primitiveData.value,
                          (UA_DataTypeKind)type->typeKind, type->memSize);
    }
    else if (type->typeKind == UA_DATATYPEKIND_NODEID)
    {
        setNodeId(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_QUALIFIEDNAME)
    {
        setQualifiedName(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_LOCALIZEDTEXT)
    {
        setLocalizedText(value, data);
    }
    else if (type->typeKind == UA_DATATYPEKIND_DATETIME)
    {
        data->offset += sizeof(UA_Int64);
    }
    else if (type->typeKind == UA_DATATYPEKIND_ENUM)
    {
        setPrimitiveValue(data, value->val.primitiveData.value,
                          UA_DATATYPEKIND_INT32,
                          UA_TYPES[UA_TYPES_INT32].memSize);
    }
    else if (type->typeKind == UA_DATATYPEKIND_STRUCTURE)
    {
        setStructure(value, type, data, customTypes);
    }
}

static void setArray(const Value *value, const UA_DataType *type, RawData *data, const UA_DataType* customTypes)
{
    for (size_t i = 0; i < value->data->val.complexData.membersSize; i++)
    {
        setScalar(value->data->val.complexData.members[i], type, data, customTypes);
    }
}

static void setData(const Value *value, const UA_DataType *type, RawData *data, const UA_DataType* customTypes)
{
    if (value->isArray)
    {
        setArray(value, type, data, customTypes);
    }
    else
    {
        setScalar(value->data, type, data, customTypes);
    }
}

RawData *Value_getData(const Value *value, const UA_DataType *type, const UA_DataType* customTypes)
{
    if (!type)
    {
        return NULL;
    }

    RawData *data = RawData_new();
    if (value->isArray)
    {
        data->mem =
            calloc(value->data->val.complexData.membersSize, type->memSize);
    }
    else
    {
        data->mem = calloc(1, type->memSize);
    }

    setData(value, type, data, customTypes);
    return data;
}
