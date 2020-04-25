#include "DataTypeImporter.h"
#include "conversion.h"
#include <assert.h>
#include <open62541/server_config.h>
#include <open62541/types.h>

#define alignof(type) offsetof(struct {char c; type d;}, d)

struct DataTypeImporter
{
    UA_DataTypeArray* types;
};

static UA_UInt32 getBinaryEncodingId(const TDataTypeNode *node)
{
    TNodeId encodingRefType = {0, "i=38"};

    Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {
        if (!TNodeId_cmp(&encodingRefType, &node->id))
        {
            UA_NodeId id = getNodeIdFromChars(ref->target);
            return id.identifier.numeric;
        }
    }
    return 0;
}

static int getAlignment(const UA_DataType *type)
{
    switch (type->typeKind)
    {
    case UA_DATATYPEKIND_BOOLEAN:
        return alignof(UA_Boolean);
    case UA_DATATYPEKIND_SBYTE:
        return alignof(UA_SByte);
    case UA_DATATYPEKIND_BYTE:
        return alignof(UA_Byte);
    case UA_DATATYPEKIND_INT16:
        return alignof(UA_Int16);
    case UA_DATATYPEKIND_UINT16:
        return alignof(UA_UInt16);
    case UA_DATATYPEKIND_INT32:
        return alignof(UA_Int32);
    case UA_DATATYPEKIND_UINT32:
        return alignof(UA_UInt32);
    case UA_DATATYPEKIND_INT64:
        return alignof(UA_Int64);
    case UA_DATATYPEKIND_UINT64:
        return alignof(UA_UInt64);
    case UA_DATATYPEKIND_FLOAT:
        return alignof(UA_Float);
    case UA_DATATYPEKIND_DOUBLE:
        return alignof(UA_Double);
    default:
        return 0;
    }
    assert(false && "typeIndex invalid");
    return 0;
}
static void setPaddingMemsize(UA_DataType *type, const UA_DataType *ns0Types,
                              const UA_DataType *customTypes)
{
    const UA_DataType *typelists[2] = {ns0Types, customTypes};
    int offset = 0;
    // everything should be there to calculate memsize, padding, etc
    for (UA_DataTypeMember *tm = type->members;
         tm < type->members + type->membersSize; tm++)
    {
        const UA_DataType *memberType =
            &typelists[!tm->namespaceZero][tm->memberTypeIndex];
        int align = getAlignment(memberType);
        if (align > 0)
        {
            tm->padding = (UA_Byte)((align - (offset % align)) % align);
        }
        else
        {
            tm->padding = 0;
        }

        offset = offset + tm->padding + memberType->memSize;
    }
    type->memSize = (UA_Byte)offset;
}

static void addDataTypeMembers(UA_DataType *type, const TDataTypeNode *node)
{
    //need casting
    type->membersSize = (unsigned char)node->definition->fieldCnt;
    type->members = (UA_DataTypeMember *)calloc(node->definition->fieldCnt,
                                                sizeof(UA_DataTypeMember));

    for (size_t i = 0; i < node->definition->fieldCnt; i++)
    {
        UA_DataTypeMember *member = type->members + i;
        member->isArray = node->definition->fields[i].valueRank >= 0;
        member->namespaceZero = node->definition->fields[i].dataType.nsIdx == 0;

        UA_NodeId memberTypeId =
            getNodeIdFromChars(node->definition->fields[i].dataType);

        // TODO: fix this, is just for testing
        member->memberTypeIndex = (UA_UInt16)(memberTypeId.identifier.numeric-1);
        char *memberNameCopy = (char *)UA_calloc(
            strlen(node->definition->fields[i].name) + 1, sizeof(char));
        memcpy(memberNameCopy, node->definition->fields[i].name,
               strlen(node->definition->fields[i].name));
        member->memberName = memberNameCopy;
    }
}

void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const TDataTypeNode *node)
{
    if(!node->definition)
    {
        return;
    }
    if (node->definition->isEnum)
    {
        return;
    }

    importer->types->types = (UA_DataType *)realloc(
        (void*)(uintptr_t)importer->types->types,
        (importer->types->typesSize + 1) * sizeof(UA_DataType));

    UA_DataType *type = (UA_DataType*)(uintptr_t)&importer->types->types[importer->types->typesSize];
    type->typeIndex = (UA_UInt16)importer->types->typesSize;
    type->binaryEncodingId = (UA_UInt16)getBinaryEncodingId(node);
    type->typeId = getNodeIdFromChars(node->id);
    if (node->definition->isEnum)
    {
        type->typeKind = UA_DATATYPEKIND_ENUM;
    }
    else
    {
        // TODO: there can be more options (OPSTRUCT)
        type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    }

    addDataTypeMembers(type, node);

    setPaddingMemsize(type, &UA_TYPES[0], importer->types->types);
    // type->typeName = node->browseName.name;
    importer->types->typesSize++;
}

DataTypeImporter *DataTypeImporter_new(struct UA_Server *server)
{
    DataTypeImporter *importer =
        (DataTypeImporter *)calloc(1, sizeof(DataTypeImporter));
    assert(importer);

    UA_DataTypeArray *newCustomTypes =
        (UA_DataTypeArray *)UA_calloc(1, sizeof(UA_DataTypeArray));

    importer->types = newCustomTypes;
    UA_Server_getConfig(server)->customDataTypes = newCustomTypes;

    return importer;
}

void DataTypeImporter_delete(DataTypeImporter *importer) { free(importer); }