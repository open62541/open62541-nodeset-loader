/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <open62541/server.h>
#include <open62541/types.h>

#include "DataTypeImporter.h"
#include "conversion.h"
#include "customDataType.h"
#include "padding.h"

#include <assert.h>

struct DataTypeImporter {
    UA_DataTypeArray *types;
    const NL_DataTypeNode **nodes;
    size_t nodesSize;
    size_t firstNewDataType;
};

static UA_NodeId
getBinaryEncodingId(const NL_DataTypeNode *node) {
    UA_NodeId encodingRefType = UA_NODEID_NUMERIC(0, 38);
    NL_Reference *ref = node->nonHierachicalRefs;
    while (ref) {
        if (UA_NodeId_equal(&encodingRefType, &ref->refType)) {
            UA_NodeId id = ref->target;
            return id;
        }
        ref = ref->next;
    }
    return UA_NODEID_NULL;
}

static const UA_DataType *
getDataType(const UA_NodeId *id, const UA_DataTypeArray *customTypes,
            const DataTypeImporter *importer) {
    const UA_DataType *type = UA_findDataType(id);
    if (type)
        return type;

    // if it is abstract, a Variant is returned
    if (id->namespaceIndex == 0)
        return &UA_TYPES[UA_TYPES_VARIANT];

    type = findCustomDataType(id, customTypes);
    if (type && importer) {
        for (size_t i = 0; i < importer->nodesSize; i++) {
            if (UA_NodeId_equal(&importer->nodes[i]->id, &type->typeId)) {
                if (strcmp(importer->nodes[i]->isAbstract, "true") == 0)
                    return &UA_TYPES[UA_TYPES_VARIANT];
            }
        }
    }
    return type;
}

typedef struct {
    char c;
    UA_Boolean member;
} TempBoolean;

typedef struct {
    char c;
    UA_SByte member;
} TempSByte;

typedef struct {
    char c;
    UA_Byte member;
} TempByte;

typedef struct {
    char c;
    UA_Int16 member;
} TempInt16;

typedef struct {
    char c;
    UA_UInt16 member;
} TempUInt16;

typedef struct {
    char c;
    UA_Int32 member;
} TempInt32;

typedef struct {
    char c;
    UA_UInt32 member;
} TempUInt32;

typedef struct {
    char c;
    UA_Int64 member;
} TempInt64;

typedef struct {
    char c;
    UA_UInt64 member;
} TempUInt64;

typedef struct {
    char c;
    UA_Float member;
} TempFloat;

typedef struct {
    char c;
    UA_Double member;
} TempDouble;

typedef struct {
    char c;
    UA_QualifiedName member;
} TempQualifiedName;

typedef struct {
    char c;
    UA_LocalizedText member;
} TempLocalizedText;

typedef struct {
    char c;
    UA_StatusCode member;
} TempStatusCode;

typedef struct {
    char c;
    UA_String member;
} TempString;

typedef struct {
    char c;
    UA_XmlElement member;
} TempXmlElement;

typedef struct {
    char c;
    UA_ByteString member;
} TempByteString;

typedef struct {
    char c;
    UA_DateTime member;
} TempDateTime;

typedef struct {
    char c;
    UA_ExpandedNodeId member;
} TempExpandedNodeId;

typedef struct {
    char c;
    UA_NodeId member;
} TempNodeId;

typedef struct {
    char c;
    UA_DiagnosticInfo member;
} TempDiagnosticInfo;

typedef struct {
    char c;
    UA_Variant member;
} TempVariant;

typedef struct {
    char c;
    UA_ExtensionObject member;
} TempExtensionObject;

typedef struct {
    char c;
    UA_Guid member;
} TempGuid;

static int
getAlignment(const UA_DataType *type,
             const UA_DataTypeArray *customTypes) {
    switch (type->typeKind)
    {
    case UA_DATATYPEKIND_BOOLEAN:
        return offsetof(TempBoolean, member);
    case UA_DATATYPEKIND_SBYTE:
        return offsetof(TempSByte, member);
    case UA_DATATYPEKIND_BYTE:
        return offsetof(TempByte, member);
    case UA_DATATYPEKIND_INT16:
        return offsetof(TempInt16, member);
    case UA_DATATYPEKIND_UINT16:
        return offsetof(TempUInt16, member);
    case UA_DATATYPEKIND_INT32:
        return offsetof(TempInt32, member);
    case UA_DATATYPEKIND_UINT32:
        return offsetof(TempUInt32, member);
    case UA_DATATYPEKIND_INT64:
        return offsetof(TempInt64, member);
    case UA_DATATYPEKIND_UINT64:
        return offsetof(TempUInt64, member);
    case UA_DATATYPEKIND_FLOAT:
        return offsetof(TempFloat, member);
    case UA_DATATYPEKIND_DOUBLE:
        return offsetof(TempDouble, member);
    case UA_DATATYPEKIND_QUALIFIEDNAME:
        return offsetof(TempQualifiedName, member);
    case UA_DATATYPEKIND_LOCALIZEDTEXT:
        return offsetof(TempLocalizedText, member);
    case UA_DATATYPEKIND_STATUSCODE:
        return offsetof(TempStatusCode, member);
    case UA_DATATYPEKIND_STRING:
        return offsetof(TempString, member);
    case UA_DATATYPEKIND_BYTESTRING:
        return offsetof(TempByteString, member);
    case UA_DATATYPEKIND_XMLELEMENT:
        return offsetof(TempXmlElement, member);
    case UA_DATATYPEKIND_DATETIME:
        return offsetof(TempDateTime, member);
    case UA_DATATYPEKIND_EXPANDEDNODEID:
        return offsetof(TempExpandedNodeId, member);
    case UA_DATATYPEKIND_NODEID:
        return offsetof(TempNodeId, member);
    case UA_DATATYPEKIND_DIAGNOSTICINFO:
        return offsetof(TempDiagnosticInfo, member);
    case UA_DATATYPEKIND_VARIANT:
        return offsetof(TempVariant, member);
    case UA_DATATYPEKIND_ENUM:
        return offsetof(TempInt32, member);
    case UA_DATATYPEKIND_EXTENSIONOBJECT:
        return offsetof(TempExtensionObject, member);
    case UA_DATATYPEKIND_UNION:
        return 0;
    case UA_DATATYPEKIND_GUID:
        return offsetof(TempGuid, member);
    case UA_DATATYPEKIND_STRUCTURE:
    case UA_DATATYPEKIND_OPTSTRUCT:
        // here we have to take a look on the first member
        assert(type->members);
        int retAlignment = 0;
        for (UA_UInt32 i = 0; i < type->membersSize; i++)
        {
            const UA_DataType *memberType = type->members[i].memberType;
            int tmp = getAlignment(memberType, customTypes);
            if (tmp > retAlignment)
            {
                retAlignment = tmp;
            }
        }
        return retAlignment;
    }

    assert(false && "typeKind not handled");
    return 0;
}

typedef struct {
    char c;
    size_t member;
} TempSizeT;

typedef struct {
    char c;
    void *member;
} TempVoidPtr;

static void
setPaddingMemsize(UA_DataType *type,
                  const UA_DataTypeArray *customTypes) {
    if (!type->members)
        return;

    int offset = 0;
    int endPadding = 0;
    UA_UInt16 biggestMemberSize = 0;
    UA_Boolean hasArrayMember = false;

    for (UA_DataTypeMember *tm = type->members;
         tm < type->members + type->membersSize; tm++) {
        const UA_DataType *memberType = tm->memberType;
        type->pointerFree = type->pointerFree && memberType->pointerFree;

        if (tm->isArray) {
            // for arrays we have to take the size_t for the arraySize into
            // account if the open changes the implementation of array
            // serialization, we have a serious problem
            // we rely here that its an size_t
            int align = offsetof(TempSizeT, member);
            tm->padding = (UA_Byte)(0x3F & getPadding(align, offset));
            offset = offset + tm->padding + (UA_Byte)sizeof(size_t);
            // the void* for data
            align = offsetof(TempVoidPtr, member);
            int padding2 = getPadding(align, offset);
            offset = offset + padding2 + (UA_Byte)sizeof(void *);
            // datatype is not pointerfree
            type->pointerFree = false;

            if (sizeof(void *) > biggestMemberSize)
                biggestMemberSize = sizeof(void *);

            hasArrayMember = true;
        } else if (tm->isOptional) {
            int align = offsetof(TempVoidPtr, member);
            tm->padding = (UA_Byte)(0x3F & getPadding(align, offset));
            offset = offset + tm->padding + (UA_Byte)sizeof(void *);
            type->pointerFree = false;
            if (sizeof(void *) > biggestMemberSize)
                biggestMemberSize = sizeof(void *);
        } else {
            // add the switchfield to the padding of the first datatype member
            if (type->typeKind == UA_DATATYPEKIND_UNION) {
                tm->padding = (UA_Byte)sizeof(UA_UInt32);
            } else if (memberType->typeKind != UA_DATATYPEKIND_UNION) {
                int align = getAlignment(memberType, customTypes);
                tm->padding = (UA_Byte)(0x3F & getPadding(align, offset));
            }

            offset = offset + tm->padding + memberType->memSize;
            // padding after struct at end is needed
            if (memberType->memSize > sizeof(size_t)) {
                endPadding = memberType->memSize % sizeof(size_t);
            }
            if (memberType->memSize > biggestMemberSize) {
                biggestMemberSize = memberType->memSize;
            }
        }
    }

    if (type->typeKind == UA_DATATYPEKIND_UNION) {
        // add the switch field
        type->memSize = sizeof(UA_Int32);
        int padding = getPadding(offsetof(TempUInt32, member), 0);
        type->memSize = (UA_UInt16)(type->memSize + padding);
        type->memSize = (UA_UInt16)(type->memSize + biggestMemberSize);
        if (hasArrayMember)
            type->memSize = (UA_UInt16)(type->memSize + sizeof(size_t));
        endPadding = getPadding(offsetof(TempSizeT, member), type->memSize);
        type->memSize = (UA_UInt16)(type->memSize + endPadding);
    } else {
        type->memSize = (UA_UInt16)(offset + endPadding);
    }
}

static UA_NodeId
getParentNode(const NL_DataTypeNode *node) {
    NL_Reference *ref = node->hierachicalRefs;
    while (ref) {
        if (!ref->isForward)
            return ref->target;
        ref = ref->next;
    }
    return UA_NODEID_NULL;
}

static void
setDataTypeMembersTypeIndex(DataTypeImporter *importer,
                            UA_DataType *type, const NL_DataTypeNode *node) {
    // member of supertype have to be added, if there is one
    UA_NodeId parent = getParentNode(node);
    UA_NodeId structId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    size_t memberOffset = 0;
    if (!UA_NodeId_equal(&parent, &structId)) {
        const UA_DataType *parentType = getDataType(&parent, importer->types, NULL);
        // copy over parent members, if no members (abstract type), nothing is
        // done
        // First need to check if parentType exists at all. NodesetCompiler in
        // open62541 would not generate a type if it had no members.
        if (parentType && parentType->members) {
            UA_DataTypeMember *members = (UA_DataTypeMember *)calloc(
                (size_t)(parentType->membersSize + type->membersSize),
                sizeof(UA_DataTypeMember));
            memcpy(members, parentType->members,
                   parentType->membersSize * sizeof(UA_DataTypeMember));

            size_t cnt = 0;
            for (UA_DataTypeMember *m = members;
                 m != members + parentType->membersSize; m++) {
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
         member != type->members + type->membersSize; member++) {
        UA_NodeId memberTypeId = node->definition->fields[i].dataType;
        member->memberType =
            getDataType(&memberTypeId, importer->types, importer);
        i++;
    }
}

static void
addDataTypeMembers(const UA_DataTypeArray *customTypes,
                   UA_DataType *type, const NL_DataTypeNode *node) {

    if (!node->definition) {
        type->membersSize = 0;
        type->members = NULL;
        type->memSize = sizeof(void *);
        return;
    }

    type->membersSize = (unsigned char)node->definition->fieldCnt;
    type->members = (UA_DataTypeMember *)calloc(node->definition->fieldCnt,
                                                sizeof(UA_DataTypeMember));

    for (size_t i = 0; i < node->definition->fieldCnt; i++) {
        UA_DataTypeMember *member = type->members + i;
        member->isArray = node->definition->fields[i].valueRank >= 0;
        UA_NodeId typeId = node->definition->fields[i].dataType;
        member->memberType = getDataType(&typeId, customTypes, NULL);

        char *memberNameCopy = (char *)UA_calloc(
            strlen(node->definition->fields[i].name) + 1, sizeof(char));
        memcpy(memberNameCopy, node->definition->fields[i].name,
               strlen(node->definition->fields[i].name));
        member->memberName = memberNameCopy;
        member->isOptional = node->definition->fields[i].isOptional;
        if (member->isOptional) {
            type->pointerFree = false;
            type->typeKind = UA_DATATYPEKIND_OPTSTRUCT;
        }
    }
}

static void
StructureDataType_init(const DataTypeImporter *importer,
                       UA_DataType *type,
                       const NL_DataTypeNode *node,
                       bool isOptionSet) {
    if (node->definition && node->definition->isUnion) {
        type->typeKind = UA_DATATYPEKIND_UNION;
    } else {
        type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    }
    type->typeId = node->id;
    type->binaryEncodingId = getBinaryEncodingId(node);
    type->pointerFree = true;
    if (!isOptionSet)
        addDataTypeMembers(importer->types, type, node);
    type->overlayable = false;
}

static void
EnumDataType_init(const DataTypeImporter *importer,
                  UA_DataType *enumType,
                  const NL_DataTypeNode *node) {
    enumType->typeId = node->id;
    enumType->typeKind = UA_DATATYPEKIND_ENUM;
    enumType->binaryEncodingId = UA_NODEID_NULL;
    enumType->pointerFree = true;
    enumType->overlayable = UA_BINARY_OVERLAYABLE_INTEGER;
    enumType->members = NULL;
    enumType->membersSize = 0;
    enumType->memSize = sizeof(UA_Int32);
}

static void
SubtypeOfBase_init(const DataTypeImporter *importer,
                   UA_DataType *type, const NL_DataTypeNode *node,
                   const UA_NodeId parent) {
    const UA_DataType *parentType = UA_findDataType(&parent);
    assert(parentType);
    type->typeId = node->id;
    type->binaryEncodingId = parentType->binaryEncodingId;
    type->members = NULL;
    type->membersSize = 0;
    type->memSize = parentType->memSize;
    type->overlayable = parentType->overlayable;
    type->pointerFree = parentType->pointerFree;
    type->typeKind = parentType->typeKind;
}

static bool
readyForMemsizeCalc(const UA_DataType *type,
                    const UA_DataType *customTypes) {
    if (type->typeKind != UA_DATATYPEKIND_STRUCTURE &&
        type->typeKind != UA_DATATYPEKIND_OPTSTRUCT)
        return true;

    if (type->membersSize == 0)
        return true;

    bool ready = true;
    for (UA_DataTypeMember *m = type->members;
         m != type->members + type->membersSize; m++) {
        if (m->memberType && m->memberType->memSize > 0)
            continue;
        ready = false;
        break;
    }
    return ready;
}

static void
calcMemSize(DataTypeImporter *importer) {
    bool allTypesFinished = false;
    // datatype nodes could be sorted upfront to detect cyclic dependencies
    while (!allTypesFinished) {
        allTypesFinished = true;
        for (UA_DataType *type =
                 (UA_DataType *)(uintptr_t)importer->types->types +
                 importer->firstNewDataType;
             type != importer->types->types + importer->types->typesSize;
             type++) {
            // we can calculate the memsize if the memsize of all membertypes is
            // known
            if (readyForMemsizeCalc(type, importer->types->types)) {
                setPaddingMemsize(type, importer->types);
            } else {
                allTypesFinished = false;
            }
        }
    }
}

void
DataTypeImporter_initMembers(DataTypeImporter *importer) {

    size_t cnt = 0;
    for (UA_DataType *type = (UA_DataType *)(uintptr_t)importer->types->types +
                             importer->firstNewDataType;
         type != importer->types->types + importer->types->typesSize; type++) {
        if (type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
            type->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
            type->typeKind == UA_DATATYPEKIND_UNION) {
            setDataTypeMembersTypeIndex(importer, type, importer->nodes[cnt]);
        }
        cnt++;
    }
    calcMemSize(importer);
}

void
DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                   const NL_DataTypeNode *node,
                                   const UA_NodeId parent) {
    // there is an open issue for that
    // the user of the library should provide the memory for the custom
    // dataTypes, then it is clear that he has to clean it up
    if (!importer->types->types) {
        importer->types->types =
            (UA_DataType *)calloc(1000, sizeof(UA_DataType));
    }
    assert(importer->types->typesSize < 1000);

    UA_DataType *type = (UA_DataType *)(uintptr_t)&importer->types
                            ->types[importer->types->typesSize];
    memset(type, 0, sizeof(UA_DataType));
    type->typeId = node->id;
    size_t len = node->browseName.name.length;
    type->typeName = (char *)calloc(len + 1, sizeof(char));
    memcpy((void*)(uintptr_t)type->typeName, node->browseName.name.data, len);

    UA_NodeId enumeration = UA_NODEID_NUMERIC(0, UA_NS0ID_ENUMERATION);
    UA_NodeId structure = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    UA_NodeId optionset = UA_NODEID_NUMERIC(0, UA_NS0ID_OPTIONSET);
    if (UA_NodeId_equal(&parent, &enumeration)) {
        EnumDataType_init(importer, type, node);
    } else if (UA_NodeId_equal(&parent, &optionset)) {
        // treat optionset like a struct
        StructureDataType_init(importer, type, node, true);
    } else if (UA_NodeId_equal(&parent, &structure)) {
        StructureDataType_init(importer, type, node, false);
    } else {
        SubtypeOfBase_init(importer, type, node, parent);
    }

    importer->nodes = (const NL_DataTypeNode **)realloc(
        (void *)importer->nodes, (importer->nodesSize + 1) * sizeof(void *));
    importer->nodes[importer->nodesSize] = node;
    importer->nodesSize++;

    (*(size_t *)(uintptr_t)&importer->types->typesSize)++;
}

DataTypeImporter *
DataTypeImporter_new(struct UA_Server *server) {
    DataTypeImporter *importer =
        (DataTypeImporter *)calloc(1, sizeof(DataTypeImporter));
    if (!importer)
        return NULL;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if (!config->customDataTypes) {
        // we append all types to customTypes array
        UA_DataTypeArray *newCustomTypes =
            (UA_DataTypeArray *)UA_calloc(1, sizeof(UA_DataTypeArray));
        if (!newCustomTypes)
            return importer;
        newCustomTypes->cleanup = UA_TRUE;
        newCustomTypes->next = config->customDataTypes;
        config->customDataTypes = newCustomTypes;
    }
    importer->types = (UA_DataTypeArray *)(uintptr_t)config->customDataTypes;
    importer->firstNewDataType = importer->types->typesSize;
    return importer;
}

void DataTypeImporter_delete(DataTypeImporter *importer) {
    free((void *)importer->nodes);
    free(importer);
}
