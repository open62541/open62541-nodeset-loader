/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_NODESETLOADER_H
#define NODESETLOADER_NODESETLOADER_H

#include <open62541/types.h>
#include <open62541/types_generated.h>

#include "Extension.h"
#include "Logger.h"
#include "ReferenceService.h"
#include "arch.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NL_NODECLASS_COUNT 8
typedef enum
{
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5,
    NODECLASS_VARIABLETYPE = 6,
    NODECLASS_VIEW = 7
    // eventtype is handled like a object type
} NL_NodeClass;

LOADER_EXPORT extern const char *NL_NODECLASS_NAME[NL_NODECLASS_COUNT];

typedef struct
{
    uint16_t nsIdx;
    char *name;
} NL_BrowseName;

struct NL_Reference;
typedef struct NL_Reference NL_Reference;

struct NL_Reference
{
    bool isForward;
    UA_NodeId refType;
    UA_NodeId target;
    NL_Reference *next;
};

struct NL_BiDirectionalReference;
typedef struct NL_BiDirectionalReference NL_BiDirectionalReference;
struct NL_BiDirectionalReference
{
    UA_NodeId source;
    UA_NodeId target;
    UA_NodeId refType;
    NL_BiDirectionalReference *next;
};

struct NL_LocalizedText
{
    char *locale;
    char *text;
};
typedef struct NL_LocalizedText NL_LocalizedText;

#define NL_NODE_ATTRIBUTES                                                        \
    NL_NodeClass nodeClass;                                                      \
    UA_NodeId id;                                                                \
    NL_BrowseName browseName;                                                    \
    NL_LocalizedText displayName;                                                \
    NL_LocalizedText description;                                                \
    char *writeMask;                                                           \
    NL_Reference *hierachicalRefs;                                                \
    NL_Reference *nonHierachicalRefs;                                             \
    NL_Reference *unknownRefs;                                                    \
    void *extension;

#define NL_NODE_INSTANCE_ATTRIBUTES UA_NodeId parentNodeId;

struct NL_Node
{
    NL_NODE_ATTRIBUTES
};
typedef struct NL_Node NL_Node;

struct NL_InstanceNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
};
typedef struct NL_InstanceNode NL_InstanceNode;

struct NL_ObjectNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
    char *eventNotifier;
    NL_Reference *refToTypeDef;
};
typedef struct NL_ObjectNode NL_ObjectNode;

struct NL_ObjectTypeNode
{
    NL_NODE_ATTRIBUTES
    char *isAbstract;
};
typedef struct NL_ObjectTypeNode NL_ObjectTypeNode;

struct NL_VariableTypeNode
{
    NL_NODE_ATTRIBUTES
    char *isAbstract;
    UA_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
};
typedef struct NL_VariableTypeNode NL_VariableTypeNode;

struct NL_Data;
typedef struct NL_Data NL_Data;
enum NL_DataType
{
    DATATYPE_PRIMITIVE,
    DATATYPE_COMPLEX,
};

typedef enum NL_DataType NL_DataType;

struct NL_PrimitiveData
{
    const char *value;
};
typedef struct NL_PrimitiveData NL_PrimitiveData;
struct NL_ComplexData
{
    size_t membersSize;
    NL_Data **members;
};
typedef struct NL_ComplexData NL_ComplexData;

struct NL_Data
{
    NL_DataType type;
    const char *name;
    union
    {
        NL_PrimitiveData primitiveData;
        NL_ComplexData complexData;
    } val;
    NL_Data *parent;
};

struct NL_ParserCtx;
struct NL_Value
{
    struct NL_ParserCtx *ctx;
    bool isArray;
    bool isExtensionObject;
    const char *type;
    UA_NodeId typeId;
    NL_Data *data;
};
typedef struct NL_Value NL_Value;
struct NL_VariableNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
    UA_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    char *historizing;
    char *minimumSamplingInterval;
    NL_Value *value;
    NL_Reference *refToTypeDef;
};
typedef struct NL_VariableNode NL_VariableNode;

typedef struct
{
    char *name;
    UA_NodeId dataType;
    int valueRank;
    int value;
    bool isOptional;
} NL_DataTypeDefinitionField;

typedef struct
{
    NL_DataTypeDefinitionField *fields;
    size_t fieldCnt;
    bool isEnum;
    bool isUnion;
    bool isOptionSet;
} NL_DataTypeDefinition;

struct NL_DataTypeNode
{
    NL_NODE_ATTRIBUTES
    NL_DataTypeDefinition *definition;
    char *isAbstract;
};
typedef struct NL_DataTypeNode NL_DataTypeNode;

struct NL_MethodNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
    char *executable;
    char *userExecutable;
};
typedef struct NL_MethodNode NL_MethodNode;

struct NL_ReferenceTypeNode
{
    NL_NODE_ATTRIBUTES
    NL_LocalizedText inverseName;
    char *symmetric;
};
typedef struct NL_ReferenceTypeNode NL_ReferenceTypeNode;

struct NL_ViewNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
    char *containsNoLoops;
    char *eventNotifier;
};
typedef struct NL_ViewNode NL_ViewNode;

typedef unsigned short (*NL_addNamespaceCallback)(void *userContext, const char *);

struct NL_FileContext
{
    void *userContext;
    const char *file;
    NL_addNamespaceCallback addNamespace;
    NodesetLoader_ExtensionInterface *extensionHandling;
};
typedef struct NL_FileContext NL_FileContext;

struct NodesetLoader;
typedef struct NodesetLoader NodesetLoader;

LOADER_EXPORT NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger,
                                               struct NL_ReferenceService *refService);
LOADER_EXPORT bool NodesetLoader_importFile(NodesetLoader *loader,
                                            const NL_FileContext *fileContext);
LOADER_EXPORT void NodesetLoader_delete(NodesetLoader *loader);
LOADER_EXPORT const NL_BiDirectionalReference *
NodesetLoader_getBidirectionalRefs(const NodesetLoader *loader);
LOADER_EXPORT bool NodesetLoader_sort(NodesetLoader *loader);
typedef void (*NodesetLoader_forEachNode_Func)(void *context, NL_Node *node);
LOADER_EXPORT size_t
NodesetLoader_forEachNode(NodesetLoader *loader, NL_NodeClass nodeClass,
                          void *context, NodesetLoader_forEachNode_Func fn);
LOADER_EXPORT bool NodesetLoader_isInstanceNode (const NL_Node *baseNode);
#ifdef __cplusplus
}
#endif
#endif
