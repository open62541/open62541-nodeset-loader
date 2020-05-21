/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "DataTypeImporter.h"
#include "conversion.h"
#include "padding.h"
#include <assert.h>
#include <open62541/server_config.h>
#include <open62541/types.h>

#define alignof(type) offsetof(struct {char c; type d;}, d)

struct DataTypeImporter
{
    UA_DataTypeArray *types;
    const TDataTypeNode **nodes;
    size_t nodesSize;
    size_t firstNewDataType;
};

static UA_UInt32 getBinaryEncodingId(const TDataTypeNode *node)
{
    TNodeId encodingRefType = {0, "i=38"};

    Reference *ref = node->nonHierachicalRefs;
    while (ref)
    {
        if (!TNodeId_cmp(&encodingRefType, &ref->refType))
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
    case UA_DATATYPEKIND_VARIANT:
        return alignof(UA_Variant);
    case UA_DATATYPEKIND_ENUM:
        return alignof(UA_Int32);
    case UA_DATATYPEKIND_STRUCTURE:
    case UA_DATATYPEKIND_OPTSTRUCT:
        // here we have to take a look on the first member
        assert(type->members);
        const UA_DataType *memberType = getTypeFromLists(
            type->members[0].namespaceZero, type->members[0].memberTypeIndex,
            ns0Types, customTypes);
        return getAlignment(memberType, ns0Types, customTypes);
    }

    assert(false && "typeKind not handled");
    return 0;
}
static void setPaddingMemsize(UA_DataType *type, const UA_DataType *ns0Types,
                              const UA_DataType *customTypes)
{
    if (!type->members)
    {
        return;
    }
    int offset = 0;
    int endPadding = 0;

    UA_UInt16 biggestMemberSize = 0;

    for (UA_DataTypeMember *tm = type->members;
         tm < type->members + type->membersSize; tm++)
    {
        const UA_DataType *memberType = getTypeFromLists(
            tm->namespaceZero, tm->memberTypeIndex, ns0Types, customTypes);
        type->pointerFree = type->pointerFree && memberType->pointerFree;

        if (tm->isArray)
        {
            // for arrays we have to take the size_t for the arraySize into
            // account if the open changes the implementation of array
            // serialization, we have a serious problem
            // we rely here that its an size_t
            int align = alignof(size_t);
            tm->padding = getPadding(align, offset);
            offset = offset + tm->padding + (UA_Byte)sizeof(size_t);
            // the void* for data
            align = alignof(void *);
            int padding2 = 0;
            padding2 = getPadding(align, offset);
            offset = offset + padding2 + (UA_Byte)sizeof(void *);
            // datatype is not pointerfree
            type->pointerFree = false;

            if (sizeof(void *) > biggestMemberSize)
            {
                biggestMemberSize = sizeof(void *);
            }
        }
        else if (tm->isOptional)
        {
            int align = alignof(void *);
            tm->padding = getPadding(align, offset);
            offset = offset + tm->padding + (UA_Byte)sizeof(void *);
            type->pointerFree = false;
            if (sizeof(void *) > biggestMemberSize)
            {
                biggestMemberSize = sizeof(void *);
            }
        }
        else
        {
            if(type->typeKind == UA_DATATYPEKIND_UNION && tm>type->members)
            {
                tm->padding = sizeof(UA_UInt32);
            }
            else
            {
                int align = getAlignment(memberType, ns0Types, customTypes);
                tm->padding = getPadding(align, offset);
                offset = offset + tm->padding + memberType->memSize;
                // padding after struct at end is needed
                if (memberType->memSize > sizeof(size_t))
                {
                    endPadding = memberType->memSize % sizeof(size_t);
                }
                if (memberType->memSize > biggestMemberSize)
                {
                    biggestMemberSize = memberType->memSize;
                }
            }
        }
    }
    if (type->typeKind == UA_DATATYPEKIND_UNION)
    {
        // add the switch field
        type->memSize = sizeof(UA_Int32);
        int padding = getPadding(alignof(UA_Int32), 0);
        type->memSize = (UA_UInt16)(type->memSize + padding);
        type->memSize = (UA_UInt16)(type->memSize + biggestMemberSize);
        endPadding = getPadding(alignof(size_t), type->memSize);
        type->memSize = (UA_UInt16)(type->memSize + endPadding);
        type->membersSize++;
    }
    else
    {
        type->memSize = (UA_UInt16)(offset + endPadding);
    }
}

static UA_UInt16 getTypeIndex(const DataTypeImporter *importer,
                              const UA_NodeId *id)
{
    if (id->namespaceIndex == 0)
    {
        const UA_DataType *ns0type = UA_findDataType(id);
        return ns0type->typeIndex;
    }
    size_t idx = 0;
    bool found = false;
    for (const UA_DataType *customType = importer->types->types;
         customType != importer->types->types + importer->types->typesSize;
         customType++)
    {
        if (UA_NodeId_equal(&customType->typeId, id))
        {
            found = true;
            break;
        }
        idx++;
    }
    assert(found && "DataTypeIndex not found");
    if (found)
    {
        return (UA_UInt16)idx;
    }
    else
    {
        return 0;
    }
}

static TNodeId getParentNode(const TDataTypeNode *node)
{
    Reference *ref = node->hierachicalRefs;
    while (ref)
    {
        if (!ref->isForward)
        {
            return ref->target;
        }
        ref=ref->next;
    }
    TNodeId nullId = {0, NULL};
    return nullId;
}

static void setDataTypeMembersTypeIndex(DataTypeImporter *importer,
                                        UA_DataType *type,
                                        const TDataTypeNode *node)
{
    // member of supertype have to be added, if there is one
    UA_NodeId parent = getNodeIdFromChars(getParentNode(node));
    UA_NodeId structId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    size_t memberOffset = 0;
    if (!UA_NodeId_equal(&parent, &structId))
    {
        UA_UInt16 idx = getTypeIndex(importer, &parent);
        const UA_DataType *parentType = NULL;
        if (parent.namespaceIndex == 0)
        {
            parentType = &UA_TYPES[idx];
        }
        else
        {
            parentType = &importer->types->types[idx];
        }
        // copy over parent members, if no members (abstract type), nothing is
        // done
        if (parentType->members)
        {
            UA_DataTypeMember *members = (UA_DataTypeMember *)calloc(
                (size_t)(parentType->membersSize + type->membersSize),
                sizeof(UA_DataTypeMember));
            memcpy(members, parentType->members,
                   parentType->membersSize * sizeof(UA_DataTypeMember));

            size_t cnt = 0;
            for (UA_DataTypeMember *m = members;
                 m != members + parentType->membersSize; m++)
            {
                char *memberNameCopy =
                    (char *)UA_calloc(strlen(m->memberName) + 1, sizeof(char));
                memcpy(memberNameCopy, m->memberName, strlen(m->memberName));
                members[cnt].memberName = memberNameCopy;
                cnt++;
            }

            memcpy(members + parentType->membersSize, type->members,
                   type->membersSize * sizeof(UA_DataTypeMember));
            free(type->members);
            type->members = members;
            type->membersSize =
                (uint8_t)(parentType->membersSize + type->membersSize);
            type->memSize = parentType->memSize;
            memberOffset = parentType->membersSize;
        }
    }

    size_t i = 0;
    for (UA_DataTypeMember *member = type->members + memberOffset;
         member != type->members + type->membersSize; member++)
    {
        UA_NodeId memberTypeId =
            getNodeIdFromChars(node->definition->fields[i].dataType);
        member->memberTypeIndex = getTypeIndex(importer, &memberTypeId);
        i++;
    }
}

static void addDataTypeMembers(const UA_DataType *customTypes,
                               UA_DataType *type, const TDataTypeNode *node)
{

    if (!node->definition)
    {
        type->membersSize = 0;
        type->members = NULL;
        type->memSize = sizeof(void *);
        return;
    }

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
        member->isOptional = node->definition->fields[i].isOptional;
        if (member->isOptional)
        {
            type->pointerFree = false;
            type->typeKind = UA_DATATYPEKIND_OPTSTRUCT;
        }
    }
}

static void StructureDataType_init(const DataTypeImporter *importer,
                                   UA_DataType *type, const TDataTypeNode *node, bool isOptionSet)
{
    if (node->definition && node->definition->isUnion)
    {
        type->typeKind = UA_DATATYPEKIND_UNION;
    }
    else
    {
        type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    }
    type->typeIndex = (UA_UInt16)importer->types->typesSize;
    type->binaryEncodingId = (UA_UInt16)getBinaryEncodingId(node);
    type->pointerFree = true;
    if(!isOptionSet)
    {
        addDataTypeMembers(importer->types->types, type, node);
    }    
    type->overlayable = false;
}

static void EnumDataType_init(const DataTypeImporter *importer,
                              UA_DataType *enumType, const TDataTypeNode *node)
{
    enumType->typeIndex = (UA_UInt16)importer->types->typesSize;
    enumType->typeKind = UA_DATATYPEKIND_ENUM;
    enumType->binaryEncodingId = 0;
    enumType->pointerFree = true;
    enumType->overlayable = UA_BINARY_OVERLAYABLE_INTEGER;
    enumType->members = NULL;
    enumType->membersSize = 0;
    enumType->memSize = sizeof(UA_Int32);
}

static void SubtypeOfBase_init(const DataTypeImporter *importer,
                               UA_DataType *type, const TDataTypeNode *node,
                               const UA_NodeId parent)
{
    const UA_DataType* parentType = UA_findDataType(&parent);

    assert(parentType);

    type->typeIndex = (UA_UInt16)importer->types->typesSize;
    type->binaryEncodingId = parentType->binaryEncodingId;
    type->members = NULL;
    type->membersSize = 0;
    type->memSize = parentType->memSize;
    type->overlayable = parentType->overlayable;
    type->pointerFree = parentType->pointerFree;
    type->typeKind = parentType->typeKind;
}

static bool readyForMemsizeCalc(const UA_DataType *type,
                                const UA_DataType *customTypes)
{
    if (type->typeKind != UA_DATATYPEKIND_STRUCTURE &&
        type->typeKind != UA_DATATYPEKIND_OPTSTRUCT)
    {
        return true;
    }
    if (type->membersSize == 0)
    {
        return true;
    }
    bool ready = true;
    for (UA_DataTypeMember *m = type->members;
         m != type->members + type->membersSize; m++)
    {
        if (m->namespaceZero)
        {
            continue;
        }
        if (customTypes[m->memberTypeIndex].memSize > 0)
        {
            continue;
        }
        ready = false;
        break;
    }
    return ready;
}

static void calcMemSize(DataTypeImporter *importer)
{
    bool allTypesFinished = false;
    // TODO: possible an endless loop
    // datatype nodes could be sorted upfront to detect cyclic dependencies
    while (!allTypesFinished)
    {
        allTypesFinished = true;
        for (UA_DataType *type =
                 (UA_DataType *)(uintptr_t)importer->types->types +
                 importer->firstNewDataType;
             type != importer->types->types + importer->types->typesSize;
             type++)
        {
            // we can calculate the memsize if the memsize of all membertypes is
            // known
            if (readyForMemsizeCalc(type, importer->types->types))
            {
                setPaddingMemsize(type, &UA_TYPES[0], importer->types->types);
            }
            else
            {
                allTypesFinished = false;
            }
        }
    }
}

void DataTypeImporter_initMembers(DataTypeImporter *importer)
{

    size_t cnt = 0;
    for (UA_DataType *type = (UA_DataType *)(uintptr_t)importer->types->types +
                             importer->firstNewDataType;
         type != importer->types->types + importer->types->typesSize; type++)
    {
        if (type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
            type->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
            type->typeKind == UA_DATATYPEKIND_UNION)
        {
            setDataTypeMembersTypeIndex(importer, type, importer->nodes[cnt]);
        }
        cnt++;
    }
    calcMemSize(importer);
}

void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const TDataTypeNode *node,
                                        const UA_NodeId parent)
{
    // TODO: can we to this in a more clever way?
    // the user of the library should provide the memory for the custom
    // dataTypes, then it is clear that he has to clean it up
    if (!importer->types->types)
    {
        importer->types->types =
            (UA_DataType *)calloc(100, sizeof(UA_DataType));
    }
    assert(importer->types->typesSize < 100);

    UA_DataType *type = (UA_DataType *)(uintptr_t)&importer->types
                            ->types[importer->types->typesSize];
    memset(type, 0, sizeof(UA_DataType));
    type->typeId = getNodeIdFromChars(node->id);
    if (node->browseName.name)
    {
        size_t len = strlen(node->browseName.name);
        type->typeName = (char *)calloc(len + 1, sizeof(char));
        memcpy((void *)(uintptr_t)type->typeName, node->browseName.name, len);
    }

    UA_NodeId enumeration = UA_NODEID_NUMERIC(0,UA_NS0ID_ENUMERATION);
    UA_NodeId structure = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    UA_NodeId optionset = UA_NODEID_NUMERIC(0, UA_NS0ID_OPTIONSET);
    if (UA_NodeId_equal(&parent, &enumeration))
    {
        EnumDataType_init(importer, type, node);
    }
    else if (UA_NodeId_equal(&parent, &optionset))
    {
        //treat optionset like a struct
        StructureDataType_init(importer, type, node, true);
    }
    
    else if (UA_NodeId_equal(&parent, &structure))
    {
        StructureDataType_init(importer, type, node, false);
    }
    else
    {
        SubtypeOfBase_init(importer, type, node, parent);
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
    if (!importer)
    {
        return NULL;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if (!config->customDataTypes)
    {
        // we append all types to customTypes array
        UA_DataTypeArray *newCustomTypes =
            (UA_DataTypeArray *)UA_calloc(1, sizeof(UA_DataTypeArray));
        if (!newCustomTypes)
        {
            return importer;
        }
        newCustomTypes->next = config->customDataTypes;
        config->customDataTypes = newCustomTypes;
    }
    importer->types = (UA_DataTypeArray *)(uintptr_t)config->customDataTypes;
    importer->firstNewDataType = importer->types->typesSize;
    return importer;
}

void DataTypeImporter_delete(DataTypeImporter *importer)
{
    free(importer->nodes);
    free(importer);
}
