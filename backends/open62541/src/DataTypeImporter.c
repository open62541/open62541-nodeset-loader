/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "internal.h"

#include <assert.h>

static UA_NodeId
getBinaryEncodingId(const NL_DataTypeNode *node) {
    UA_NodeId encodingRefType = UA_NODEID_NUMERIC(0, 38);
    for(NL_Reference *ref = node->refs; ref; ref = ref->next) {
        if(!UA_NodeId_equal(&encodingRefType, &ref->refType))
            continue;
        if(!ref->targetPtr)
            continue;
        UA_String binaryStr = UA_STRING("Default Binary");
        if(!UA_String_equal(&ref->targetPtr->browseName.name, &binaryStr))
            continue;
        UA_NodeId idCopy;
        UA_NodeId_copy(&ref->target, &idCopy);
        return idCopy;
    }
    return UA_NODEID_NULL;
}

static UA_NodeId
getXmlEncodingId(const NL_DataTypeNode *node) {
    UA_NodeId encodingRefType = UA_NODEID_NUMERIC(0, 38);
    for(NL_Reference *ref = node->refs; ref; ref = ref->next) {
        if(!UA_NodeId_equal(&encodingRefType, &ref->refType))
            continue;
        if(!ref->targetPtr)
            continue;
        UA_String xmlStr = UA_STRING("Default XML");
        if(!UA_String_equal(&ref->targetPtr->browseName.name, &xmlStr))
            continue;
        UA_NodeId idCopy;
        UA_NodeId_copy(&ref->target, &idCopy);
        return idCopy;
    }
    return UA_NODEID_NULL;
}

static const UA_DataType *
getDataType(AddNodeContext *ctx, const UA_NodeId *id) {
    return UA_Server_findDataType(ctx->server, id);
}

static UA_StatusCode
addDataTypeMembers(AddNodeContext *ctx, UA_DataType *type,
                   const NL_DataTypeNode *node,
                   const UA_NodeId *parent) {
    // Get the parent type
    const UA_DataType *parentType = getDataType(ctx, parent);
    size_t memberSize = 0;
    if(node->definition)
        memberSize += node->definition->fieldCnt;
    if(parentType)
        memberSize += parentType->membersSize;

    // Allocate the members
    type->members = (UA_DataTypeMember *)
        calloc(memberSize, sizeof(UA_DataTypeMember));
    if(!type->members)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    type->membersSize = (unsigned char)memberSize;

    // Copy over members from the parent
    size_t i = 0;
    if(parentType) {
        for(; i < parentType->membersSize; i++) {
            const UA_DataTypeMember *src = &parentType->members[i];
            UA_DataTypeMember *dst = &type->members[i];
            size_t nameLen = strlen(src->memberName);
            char *memberName = (char *)UA_calloc(1, nameLen + 1);
            memcpy(memberName, src->memberName, nameLen);
            dst->memberName = memberName;
            dst->memberType = src->memberType;

            // Pointer free??
            if(!dst->memberType->pointerFree)
                type->pointerFree = false;

            // Array?
            dst->isArray = src->isArray;
            if(dst->isArray)
                type->pointerFree = false;

            // Optional?
            dst->isOptional = src->isOptional;
            if(dst->isOptional) {
                type->pointerFree = false;
                type->typeKind = UA_DATATYPEKIND_OPTSTRUCT;
            }
        }
    }

    // Fill new member definition
    for(size_t j = 0; i < memberSize; i++, j++) {
        UA_DataTypeMember *member = &type->members[i];
        NL_DataTypeDefinitionField *field = &node->definition->fields[j];

        // Member name
        size_t nameLen = strlen(field->name);
        char *memberName = (char *)UA_calloc(1, nameLen + 1);
        memcpy(memberName, field->name, nameLen);
        member->memberName = memberName;

        // Member type
        member->memberType = getDataType(ctx, &field->dataType);
        if(!member->memberType) {
            ctx->logger->log(ctx->logger->context, NODESETLOADER_LOGLEVEL_WARNING,
                             "Cannot find member type %N of datatype %N",
                             field->dataType, type->typeId);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        // Pointer free??
        if(!member->memberType->pointerFree)
            type->pointerFree = false;

        // Array?
        member->isArray = (field->valueRank >= 0);
        if(member->isArray)
            type->pointerFree = false;

        // Optional?
        member->isOptional = field->isOptional;
        if(member->isOptional) {
            type->pointerFree = false;
            type->typeKind = UA_DATATYPEKIND_OPTSTRUCT;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
StructureDataType_init(AddNodeContext *ctx, UA_DataType *type,
                       const NL_DataTypeNode *node,
                       const UA_NodeId *parent) {
    if(node->definition->isUnion) {
        type->typeKind = UA_DATATYPEKIND_UNION;
    } else {
        type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    }
    type->typeId = node->id;
    type->pointerFree = true; // Gets overridden if necessary
    type->overlayable = false;
    return addDataTypeMembers(ctx, type, node, parent);
}

static UA_StatusCode
addEnumMembers(AddNodeContext *ctx, UA_DataType *type,
               const NL_DataTypeNode *node) {
    if(node->definition->fieldCnt == 0)
        return UA_STATUSCODE_GOOD;

    // Allocate the members
    type->members = (UA_DataTypeMember *)
        calloc(node->definition->fieldCnt, sizeof(UA_DataTypeMember));
    if(!type->members)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    type->membersSize = (UA_Byte)node->definition->fieldCnt;

    // Fill new member definition
    for(UA_Byte i = 0; i < type->membersSize; i++) {
        UA_DataTypeMember *member = &type->members[i];
        NL_DataTypeDefinitionField *field = &node->definition->fields[i];
        member->memberType = (const UA_DataType*)(uintptr_t)field->value;
        size_t nameLen = strlen(field->name);
        char *memberName = (char *)UA_calloc(1, nameLen + 1);
        memcpy(memberName, field->name, nameLen);
        member->memberName = memberName;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
EnumDataType_init(AddNodeContext *ctx, UA_DataType *enumType,
                  const NL_DataTypeNode *node,
                  const UA_NodeId *parent) {
    enumType->typeKind = UA_DATATYPEKIND_ENUM;
    enumType->pointerFree = true;
    enumType->overlayable = UA_BINARY_OVERLAYABLE_INTEGER;
    enumType->memSize = sizeof(UA_Int32);
    if(node->definition->isOptionSet) {
        const UA_DataType *parentType = getDataType(ctx, parent);
        if(parentType && parentType->typeKind <= UA_DATATYPEKIND_DOUBLE) {
            enumType->typeKind = parentType->typeKind;
            enumType->overlayable = parentType->overlayable;
            enumType->memSize = parentType->memSize;
        }
    }
    return addEnumMembers(ctx, enumType, node);
}

static UA_StatusCode
SubtypeOfBase_init(AddNodeContext *ctx,
                   UA_DataType *type, const NL_DataTypeNode *node,
                   const UA_NodeId *parent) {
    const UA_DataType *parentType = getDataType(ctx, parent);
    if(!parentType)
        return UA_STATUSCODE_BADINTERNALERROR;
    type->memSize = parentType->memSize;
    type->overlayable = parentType->overlayable;
    type->pointerFree = parentType->pointerFree;
    type->typeKind = parentType->typeKind;
    if(parentType->typeKind == UA_DATATYPEKIND_STRUCTURE ||
       parentType->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
       parentType->typeKind == UA_DATATYPEKIND_UNION) {
        return addDataTypeMembers(ctx, type, node, parent);
    } else {
        type->binaryEncodingId = parentType->binaryEncodingId;
    }
    return UA_STATUSCODE_GOOD;
}

void
addCustomDataType(AddNodeContext *ctx, const NL_DataTypeNode *node) {
    UA_DataType type;
    memset(&type, 0, sizeof(UA_DataType));

    UA_NodeId_copy(&node->id, &type.typeId);

    size_t len = node->browseName.name.length;
    type.typeName = (char *)calloc(len + 1, sizeof(char));
    memcpy((void*)(uintptr_t)type.typeName, node->browseName.name.data, len);

    UA_NodeId tmp = getBinaryEncodingId(node);
    UA_NodeId_copy(&tmp, &type.binaryEncodingId);
    tmp = getXmlEncodingId(node);
    UA_NodeId_copy(&tmp, &type.xmlEncodingId);

    UA_StatusCode res;
    UA_ExtensionObject eo;
    UA_NodeId parent = getParentId(ctx, (const NL_Node*)node, NULL);

    if(node->definition &&
       (node->definition->isEnum ||
        node->definition->isOptionSet)) {
        // Enum and OptionSet
        res = EnumDataType_init(ctx, &type, node, &parent);
    } else if(node->definition && node->definition->fieldCnt > 0) {
        // Structure and Union
        res = StructureDataType_init(ctx, &type, node, &parent);
    } else {
        // Opaque subtype
        res = SubtypeOfBase_init(ctx, &type, node, &parent);
    }
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    // Convert type to description. This generates the correct padding between
    // member fields in UA_Server_addDataTypeFromDescription.

    res = UA_DataType_toDescription(&type, &eo);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_Server_addDataTypeFromDescription(ctx->server, &eo);
    UA_ExtensionObject_clear(&eo);

 cleanup:
    if(res != UA_STATUSCODE_GOOD) {
        ctx->logger->log(ctx->logger->context, NODESETLOADER_LOGLEVEL_WARNING,
                         "Cannot create datatype description for %Ni (%s)",
                         node->id, UA_StatusCode_name(res));
    }
    UA_DataType_clear(&type);
}
