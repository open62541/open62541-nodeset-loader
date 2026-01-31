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
        if(UA_NodeId_equal(&encodingRefType, &ref->refType))
            return ref->target;
    }
    return UA_NODEID_NULL;
}

static const UA_DataType *
getDataType(AddNodeContext *ctx, const UA_NodeId *id) {
    return UA_Server_findDataType(ctx->server, id);
}

static void
addDataTypeMembers(AddNodeContext *ctx, UA_DataType *type,
                   const NL_DataTypeNode *node) {
    // No members
    if(!node->definition) {
        type->memSize = sizeof(void *);
        return;
    }

    // Allocate the members
    type->membersSize = (unsigned char)node->definition->fieldCnt;
    type->members = (UA_DataTypeMember *)
        calloc(node->definition->fieldCnt, sizeof(UA_DataTypeMember));

    // Fill member definition
    for(size_t i = 0; i < node->definition->fieldCnt; i++) {
        UA_DataTypeMember *member = &type->members[i];
        NL_DataTypeDefinitionField *field = &node->definition->fields[i];

        // Member type
        member->memberType = getDataType(ctx, &field->dataType);

        // Member name
        size_t nameLen = strlen(field->name);
        char *memberName = (char *)UA_calloc(1, nameLen + 1);
        memcpy(memberName, field->name, nameLen);
        member->memberName = memberName;

        // Array?
        member->isArray = (node->definition->fields[i].valueRank >= 0);

        // Optional?
        member->isOptional = node->definition->fields[i].isOptional;
        if(member->isOptional) {
            type->pointerFree = false;
            type->typeKind = UA_DATATYPEKIND_OPTSTRUCT;
        }
    }
}

static void
StructureDataType_init(AddNodeContext *ctx, UA_DataType *type,
                       const NL_DataTypeNode *node, bool isOptionSet) {
    if(node->definition && node->definition->isUnion) {
        type->typeKind = UA_DATATYPEKIND_UNION;
    } else {
        type->typeKind = UA_DATATYPEKIND_STRUCTURE;
    }
    type->typeId = node->id;
    type->binaryEncodingId = getBinaryEncodingId(node);
    type->pointerFree = true; // Gets overridden if necessary
    if(!isOptionSet)
        addDataTypeMembers(ctx, type, node);
    type->overlayable = false;
}

static void
EnumDataType_init(UA_DataType *enumType) {
    enumType->typeKind = UA_DATATYPEKIND_ENUM;
    enumType->pointerFree = true;
    enumType->overlayable = UA_BINARY_OVERLAYABLE_INTEGER;
    enumType->memSize = sizeof(UA_Int32);
}

static void
SubtypeOfBase_init(AddNodeContext *ctx,
                   UA_DataType *type, const NL_DataTypeNode *node,
                   const UA_NodeId parent) {
    const UA_DataType *parentType = getDataType(ctx, &parent);
    if(!parentType)
        return;
    type->binaryEncodingId = parentType->binaryEncodingId;
    type->memSize = parentType->memSize;
    type->overlayable = parentType->overlayable;
    type->pointerFree = parentType->pointerFree;
    type->typeKind = parentType->typeKind;
}

void
addCustomDataType(AddNodeContext *ctx, const NL_DataTypeNode *node) {
    UA_DataType type;
    memset(&type, 0, sizeof(UA_DataType));

    UA_NodeId_copy(&node->id, &type.typeId);

    size_t len = node->browseName.name.length;
    type.typeName = (char *)calloc(len + 1, sizeof(char));
    memcpy((void*)(uintptr_t)type.typeName, node->browseName.name.data, len);

    UA_NodeId enumeration = UA_NS0ID(ENUMERATION);
    UA_NodeId structure = UA_NS0ID(STRUCTURE);
    UA_NodeId optionset = UA_NS0ID(OPTIONSET);

    UA_NodeId parent = getParentId(ctx, (const NL_Node*)node, NULL);
    if(UA_NodeId_equal(&parent, &enumeration)) {
        EnumDataType_init(&type);
    } else if (UA_NodeId_equal(&parent, &optionset)) {
        StructureDataType_init(ctx, &type, node, true); // treat optionset like a struct
    } else if (UA_NodeId_equal(&parent, &structure)) {
        StructureDataType_init(ctx, &type, node, false);
    } else {
        SubtypeOfBase_init(ctx, &type, node, parent);
    }

    // Convert type to description. This generates the correct padding between
    // member fields in UA_Server_addDataTypeFromDescription.

    UA_StatusCode res;
    UA_ExtensionObject eo;
    res = UA_DataType_toDescription(&type, &eo);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    UA_Server_addDataTypeFromDescription(ctx->server, &eo);

 cleanup:
    UA_ExtensionObject_clear(&eo);
    UA_DataType_clear(&type);
}
