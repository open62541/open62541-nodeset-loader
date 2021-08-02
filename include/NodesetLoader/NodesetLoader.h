/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_NODESETLOADER_H
#define NODESETLOADER_NODESETLOADER_H
#include "Extension.h"
#include "Logger.h"
#include "ReferenceService.h"
#include "NodeId.h"
#include "arch.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

struct Reference;
typedef struct Reference Reference;

struct Reference
{
    bool isForward;
    NL_NodeId refType;
    NL_NodeId target;
    Reference *next;
};

struct NL_BiDirectionalReference;
typedef struct NL_BiDirectionalReference NL_BiDirectionalReference;
struct NL_BiDirectionalReference
{
    NL_NodeId source;
    NL_NodeId target;
    NL_NodeId refType;
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
    NL_NodeId id;                                                                \
    NL_BrowseName browseName;                                                    \
    NL_LocalizedText displayName;                                                \
    NL_LocalizedText description;                                                \
    char *writeMask;                                                           \
    Reference *hierachicalRefs;                                                \
    Reference *nonHierachicalRefs;                                             \
    Reference *unknownRefs;                                                    \
    void *extension;

#define NL_NODE_INSTANCE_ATTRIBUTES NL_NodeId parentNodeId;

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
    Reference *refToTypeDef;
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
    NL_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
};
typedef struct NL_VariableTypeNode NL_VariableTypeNode;

struct Data;
typedef struct Data Data;
enum DataType
{
    DATATYPE_PRIMITIVE,
    DATATYPE_COMPLEX,
};

typedef enum DataType DataType;

struct PrimitiveData
{
    const char *value;
};
typedef struct PrimitiveData PrimitiveData;
struct ComplexData
{
    size_t membersSize;
    Data **members;
};
typedef struct ComplexData ComplexData;

struct Data
{
    DataType type;
    const char *name;
    union
    {
        PrimitiveData primitiveData;
        ComplexData complexData;
    } val;
    Data *parent;
};

struct ParserCtx;
struct Value
{
    struct ParserCtx *ctx;
    bool isArray;
    bool isExtensionObject;
    const char *type;
    NL_NodeId typeId;
    Data *data;
};
typedef struct Value Value;
struct NL_VariableNode
{
    NL_NODE_ATTRIBUTES
    NL_NODE_INSTANCE_ATTRIBUTES
    NL_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    char *historizing;
    Value *value;
    Reference *refToTypeDef;
};
typedef struct NL_VariableNode NL_VariableNode;

typedef struct
{
    char *name;
    NL_NodeId dataType;
    int valueRank;
    int value;
    bool isOptional;
} DataTypeDefinitionField;

typedef struct
{
    DataTypeDefinitionField *fields;
    size_t fieldCnt;
    bool isEnum;
    bool isUnion;
    bool isOptionSet;
} DataTypeDefinition;

struct NL_DataTypeNode
{
    NL_NODE_ATTRIBUTES
    DataTypeDefinition *definition;
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

typedef int (*NL_addNamespaceCallback)(void *userContext, const char *);

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
