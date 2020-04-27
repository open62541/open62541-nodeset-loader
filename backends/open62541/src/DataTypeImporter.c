#include "DataTypeImporter.h"
#include "conversion.h"
#include <assert.h>
#include <open62541/server_config.h>
#include <open62541/types.h>

#define alignof(type) offsetof(struct {char c; type d;}, d)

struct DataTypeImporter
{
    UA_DataTypeArray *types;
    const TDataTypeNode **nodes;
    size_t nodesSize;
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
        ref = ref->next;
    }
    return 0;
}

static const UA_DataType *getTypeFromLists(bool nsZero, UA_UInt16 idx,
                                           const UA_DataType *ns0Types,
                                           const UA_DataType *customTypes)
{
    const UA_DataType *typelists[2] = {ns0Types, customTypes};
    return &typelists[!nsZero][idx];
}

static int getAlignment(const UA_DataType *type, const UA_DataType *ns0Types,
                        const UA_DataType *customTypes)
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
    case UA_DATATYPEKIND_QUALIFIEDNAME:
        return alignof(UA_QualifiedName);
    case UA_DATATYPEKIND_LOCALIZEDTEXT:
        return alignof(UA_LocalizedText);
    case UA_DATATYPEKIND_STATUSCODE:
        return alignof(UA_StatusCode);
    case UA_DATATYPEKIND_STRING:
        return alignof(UA_String);
    case UA_DATATYPEKIND_BYTESTRING:
        return alignof(UA_ByteString);
    case UA_DATATYPEKIND_DATETIME:
        return alignof(UA_DateTime);
    case UA_DATATYPEKIND_EXPANDEDNODEID:
        return alignof(UA_ExpandedNodeId);
    case UA_DATATYPEKIND_NODEID:
        return alignof(UA_NodeId);
    case UA_DATATYPEKIND_DIAGNOSTICINFO:
        return alignof(UA_DiagnosticInfo);
    case UA_DATATYPEKIND_STRUCTURE:
    case UA_DATATYPEKIND_OPTSTRUCT:
        // here we have to take a look on the first member
        assert(type->members);
        const UA_DataType *memberType = getTypeFromLists(
            type->members[0].namespaceZero, type->members[0].memberTypeIndex,
            ns0Types, customTypes);
        return getAlignment(memberType, ns0Types, customTypes);
    }

    assert(false && "typeIndex invalid");
    return 0;
}
static void setPaddingMemsize(UA_DataType *type, const UA_DataType *ns0Types,
                              const UA_DataType *customTypes)
{
    int offset = 0;
    for (UA_DataTypeMember *tm = type->members;
         tm < type->members + type->membersSize; tm++)
    {
        const UA_DataType *memberType = getTypeFromLists(
            tm->namespaceZero, tm->memberTypeIndex, ns0Types, customTypes);
        if (!tm->isArray)
        {
            int align = getAlignment(memberType, ns0Types, customTypes);
            tm->padding = (UA_Byte)((align - (offset % align)) % align);
            offset = offset + tm->padding + memberType->memSize;
        }
        else
        {
            // for arrays we have to take the size_t for the arraySize into
            // account if the open changes the implementation of array
            // serialization, we have a serious problem
            // we rely here that its an size_t
            int align = alignof(size_t);
            tm->padding = (UA_Byte)((align - (offset % align)) % align);
            offset = offset + tm->padding + (UA_Byte)sizeof(size_t);
            // the void* for data
            align = alignof(void *);
            int padding2 = 0;
            padding2 = (UA_Byte)((align - (offset % align)) % align);
            offset = offset + padding2 + (UA_Byte)sizeof(void *);
            // datatype is not pointerfree
            type->pointerFree = false;
        }
    }
    type->memSize = (UA_Byte)offset;
}

static void setDataTypeMembersTypeIndex(DataTypeImporter *importer,
                                        UA_DataType *type,
                                        const TDataTypeNode *node)
{
    size_t i = 0;
    for (UA_DataTypeMember *member = type->members;
         member != type->members + type->membersSize; member++)
    {
        // TODO: fix this, is just for testing

        UA_NodeId memberTypeId =
            getNodeIdFromChars(node->definition->fields[i].dataType);

        if (member->namespaceZero)
        {
            member->memberTypeIndex =
                (UA_UInt16)(memberTypeId.identifier.numeric - 1);
        }
        else
        {
            size_t idx = 0;
            bool found = false;
            for (const UA_DataType *customType = importer->types->types;
                 customType !=
                 importer->types->types + importer->types->typesSize;
                 customType++)
            {
                if (UA_NodeId_equal(&customType->typeId, &memberTypeId))
                {
                    found = true;
                    break;
                }
                idx++;
            }
            assert(found);
            member->memberTypeIndex = (UA_UInt16)idx;
        }
        i++;
    }
}

static void addDataTypeMembers(const UA_DataType *customTypes,
                               UA_DataType *type, const TDataTypeNode *node)
{
    // need casting here
    type->membersSize = (unsigned char)node->definition->fieldCnt;
    type->members = (UA_DataTypeMember *)calloc(node->definition->fieldCnt,
                                                sizeof(UA_DataTypeMember));

    for (size_t i = 0; i < node->definition->fieldCnt; i++)
    {
        UA_DataTypeMember *member = type->members + i;
        member->isArray = node->definition->fields[i].valueRank >= 0;
        member->namespaceZero = node->definition->fields[i].dataType.nsIdx == 0;

        char *memberNameCopy = (char *)UA_calloc(
            strlen(node->definition->fields[i].name) + 1, sizeof(char));
        memcpy(memberNameCopy, node->definition->fields[i].name,
               strlen(node->definition->fields[i].name));
        member->memberName = memberNameCopy;
    }
}

static void StructureDataType_init(const DataTypeImporter *importer,
                                   UA_DataType *type, const TDataTypeNode *node)
{
    // TODO: there can be more options (OPSTRUCT)?
    type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    type->typeIndex = (UA_UInt16)importer->types->typesSize;
    type->binaryEncodingId = (UA_UInt16)getBinaryEncodingId(node);

    // TODO: when is this true, when there are no arrays inside?
    type->pointerFree = true;
    // TODO: type->overlayable
    addDataTypeMembers(importer->types->types, type, node);

    // type->typeName = node->browseName.name;
}

static void EnumDataType_init(UA_DataType *enumType, const TDataTypeNode *node)
{
    enumType->typeIndex = UA_TYPES_INT32;
    enumType->typeKind = UA_DATATYPEKIND_ENUM;
    enumType->binaryEncodingId = 0;
    enumType->pointerFree = true;
    enumType->overlayable = UA_BINARY_OVERLAYABLE_INTEGER;
    enumType->members = NULL;
    enumType->membersSize = 0;
    enumType->memSize = sizeof(UA_Int32);
}

void DataTypeImporter_initTypes(DataTypeImporter *importer)
{
    size_t cnt = 0;
    for (UA_DataType *type = (UA_DataType *)(uintptr_t)importer->types->types;
         type != importer->types->types + importer->types->typesSize; type++)
    {
        if (type->typeKind == UA_DATATYPEKIND_STRUCTURE)
        {
            setDataTypeMembersTypeIndex(importer, type, importer->nodes[cnt]);
            setPaddingMemsize(type, &UA_TYPES[0], importer->types->types);
        }
        cnt++;
    }
}

void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const TDataTypeNode *node)
{
    if (!node->definition)
    {
        return;
    }
    importer->types->types = (UA_DataType *)realloc(
        (void *)(uintptr_t)importer->types->types,
        (importer->types->typesSize + 1) * sizeof(UA_DataType));

    UA_DataType *type = (UA_DataType *)(uintptr_t)&importer->types
                            ->types[importer->types->typesSize];
    type->typeId = getNodeIdFromChars(node->id);

    if (node->definition->isEnum)
    {
        EnumDataType_init(type, node);
    }
    else
    {
        StructureDataType_init(importer, type, node);
    }

    importer->nodes = (const TDataTypeNode **)realloc(
        importer->nodes, (importer->nodesSize + 1) * sizeof(void *));
    importer->nodes[importer->nodesSize] = node;
    importer->nodesSize++;

    (*(size_t *)(uintptr_t)&importer->types->typesSize)++;
}

DataTypeImporter *DataTypeImporter_new(struct UA_Server *server)
{
    DataTypeImporter *importer =
        (DataTypeImporter *)calloc(1, sizeof(DataTypeImporter));
    assert(importer);

    UA_DataTypeArray *newCustomTypes =
        (UA_DataTypeArray *)UA_calloc(1, sizeof(UA_DataTypeArray));

    UA_ServerConfig *config = UA_Server_getConfig(server);
    newCustomTypes->next = config->customDataTypes;

    importer->types = newCustomTypes;
    config->customDataTypes = newCustomTypes;

    return importer;
}

void DataTypeImporter_delete(DataTypeImporter *importer)
{
    free(importer->nodes);
    free(importer);
}
