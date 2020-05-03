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
    // todo: check this for dangling pointers
    s->data = (UA_Byte *)(uintptr_t)value;
}

static void setDateTime(uintptr_t adr, const char *value)
{
    printf("DateTime: %s currently not handled\n", value);
}

static void setNodeId(uintptr_t adr, const char *value)
{
    // todo: namespaceIndex should be converted?
    //*(UA_NodeId *)adr = extractNodeId(value); // getNodeIdFromChars(value);
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
    setNodeId,         setNotImplemented, setNotImplemented, setNotImplemented,
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

void RawData_delete(RawData *data) { free(data); }

static void setPrimitiveValue(RawData *data, const char *value,
                              UA_DataTypeKind kind, size_t memSize)
{
    uintptr_t adr = (uintptr_t)data->mem + data->offset;
    setScalarValueWithAddress(adr, kind, value);
    data->offset = data->offset + memSize;
}

static void setQualifiedName(const Data *value, RawData *data)
{
    assert(value->val.complexData.membersSize == 2);

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
    assert(value->val.complexData.membersSize == 2);
    setScalarValueWithAddress(
        data->offset + (uintptr_t) & ((UA_LocalizedText *)data->mem)->locale,
        UA_DATATYPEKIND_STRING,
        value->val.complexData.members[0]->val.primitiveData.value);

    setScalarValueWithAddress(
        data->offset + (uintptr_t) & ((UA_LocalizedText *)data->mem)->text,
        UA_DATATYPEKIND_STRING,
        value->val.complexData.members[0]->val.primitiveData.value);

    data->offset += sizeof(UA_LocalizedText);
}

static void setScalar(const Data *value, const UA_DataType *type,
                      RawData *data);

static void setStructure(const Data *value, const UA_DataType *type,
                         RawData *data)
{
    assert(value->type == DATATYPE_COMPLEX);
    assert(value->val.complexData.membersSize == type->membersSize);
    size_t cnt = 0;
    for (const UA_DataTypeMember *m = type->members;
         m != type->members + type->membersSize; m++)
    {
        const UA_DataType *memberType = &UA_TYPES[m->memberTypeIndex];
        setScalar(value->val.complexData.members[cnt], memberType, data);
        data->offset += m->padding;
        cnt++;
    }
}

static void setScalar(const Data *value, const UA_DataType *type, RawData *data)
{
    if (type->typeKind <= UA_DATATYPEKIND_STRING)
    {
        setPrimitiveValue(data, value->val.primitiveData.value,
                          (UA_DataTypeKind)type->typeKind, type->memSize);
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
        data->offset+=sizeof(UA_Int64);
    }
    else if (type->typeKind == UA_DATATYPEKIND_ENUM)
    {
        setPrimitiveValue(data, value->val.primitiveData.value, UA_DATATYPEKIND_INT32, UA_TYPES[UA_TYPES_INT32].memSize);
    }
    else if (type->typeKind == UA_DATATYPEKIND_STRUCTURE)
    {
        setStructure(value, type, data);
    }
}

static void setArray(const Value *value, const UA_DataType *type, RawData *data)
{
    for (size_t i = 0; i < value->data->val.complexData.membersSize; i++)
    {
        setScalar(value->data->val.complexData.members[i], type, data);
    }
}

static void setData(const Value *value, const UA_DataType *type, RawData *data)
{
    if (value->isArray)
    {
        setArray(value, type, data);
    }
    else
    {
        setScalar(value->data, type, data);
    }
}

RawData *Value_getData(const TNode *node, const Value *value)
{
    UA_NodeId dataTypeId =
        getNodeIdFromChars(((const TVariableNode *)node)->datatype);

    // check for matching type ids
    // UA_NodeId valueTypeId = getNodeIdFromChars(value->typeId);

    const UA_DataType *type = UA_findDataType(&dataTypeId);

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

    setData(value, type, data);
    return data;
}
