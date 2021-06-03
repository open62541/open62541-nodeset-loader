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
#include "TNodeId.h"
#include "arch.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NODECLASS_COUNT 8
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
} TNodeClass;

LOADER_EXPORT extern const char *NODECLASS_NAME[NODECLASS_COUNT];

typedef struct
{
    uint16_t nsIdx;
    char *name;
} TBrowseName;

struct Reference;
typedef struct Reference Reference;

struct Reference
{
    bool isForward;
    TNodeId refType;
    TNodeId target;
    Reference *next;
};

struct BiDirectionalReference;
typedef struct BiDirectionalReference BiDirectionalReference;
struct BiDirectionalReference
{
    TNodeId source;
    TNodeId target;
    TNodeId refType;
    BiDirectionalReference *next;
};

struct TLocalizedText
{
    char *locale;
    char *text;
};
typedef struct TLocalizedText TLocalizedText;

#define NODE_ATTRIBUTES                                                        \
    TNodeClass nodeClass;                                                      \
    TNodeId id;                                                                \
    TBrowseName browseName;                                                    \
    TLocalizedText displayName;                                                \
    TLocalizedText description;                                                \
    char *writeMask;                                                           \
    Reference *hierachicalRefs;                                                \
    Reference *nonHierachicalRefs;                                             \
    Reference *unknownRefs;                                                    \
    void *extension;

#define NODE_INSTANCE_ATTRIBUTES TNodeId parentNodeId;

struct TNode
{
    NODE_ATTRIBUTES
};
typedef struct TNode TNode;

struct TInstanceNode
{
    NODE_ATTRIBUTES
    NODE_INSTANCE_ATTRIBUTES
};
typedef struct TInstanceNode TInstanceNode;

struct TObjectNode
{
    NODE_ATTRIBUTES
    NODE_INSTANCE_ATTRIBUTES
    char *eventNotifier;
    Reference *refToTypeDef;
};
typedef struct TObjectNode TObjectNode;

struct TObjectTypeNode
{
    NODE_ATTRIBUTES
    char *isAbstract;
};
typedef struct TObjectTypeNode TObjectTypeNode;

struct TVariableTypeNode
{
    NODE_ATTRIBUTES
    char *isAbstract;
    TNodeId datatype;
    char *arrayDimensions;
    char *valueRank;
};
typedef struct TVariableTypeNode TVariableTypeNode;

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
    TNodeId typeId;
    Data *data;
};
typedef struct Value Value;
struct TVariableNode
{
    NODE_ATTRIBUTES
    NODE_INSTANCE_ATTRIBUTES
    TNodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    char *historizing;
    Value *value;
    Reference *refToTypeDef;
};
typedef struct TVariableNode TVariableNode;

typedef struct
{
    char *name;
    TNodeId dataType;
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

struct TDataTypeNode
{
    NODE_ATTRIBUTES
    DataTypeDefinition *definition;
    char *isAbstract;
};
typedef struct TDataTypeNode TDataTypeNode;

struct TMethodNode
{
    NODE_ATTRIBUTES
    NODE_INSTANCE_ATTRIBUTES
    char *executable;
    char *userExecutable;
};
typedef struct TMethodNode TMethodNode;

struct TReferenceTypeNode
{
    NODE_ATTRIBUTES
    TLocalizedText inverseName;
    char *symmetric;
};
typedef struct TReferenceTypeNode TReferenceTypeNode;

struct TViewNode
{
    NODE_ATTRIBUTES
    NODE_INSTANCE_ATTRIBUTES
    char *containsNoLoops;
    char *eventNotifier;
};
typedef struct TViewNode TViewNode;

typedef int (*addNamespaceCb)(void *userContext, const char *);

struct FileContext
{
    void *userContext;
    const char *file;
    addNamespaceCb addNamespace;
    NodesetLoader_ExtensionInterface *extensionHandling;
};
typedef struct FileContext FileContext;

struct NodesetLoader;
typedef struct NodesetLoader NodesetLoader;

LOADER_EXPORT NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger,
                                               struct RefService *refService);
LOADER_EXPORT bool NodesetLoader_importFile(NodesetLoader *loader,
                                            const FileContext *fileContext);
LOADER_EXPORT void NodesetLoader_delete(NodesetLoader *loader);
LOADER_EXPORT const BiDirectionalReference *
NodesetLoader_getBidirectionalRefs(const NodesetLoader *loader);
LOADER_EXPORT bool NodesetLoader_sort(NodesetLoader *loader);
typedef void (*NodesetLoader_forEachNode_Func)(void *context, TNode *node);
LOADER_EXPORT size_t
NodesetLoader_forEachNode(NodesetLoader *loader, TNodeClass nodeClass,
                          void *context, NodesetLoader_forEachNode_Func fn);
LOADER_EXPORT bool NodesetLoader_isInstanceNode (const TNode *baseNode);
#ifdef __cplusplus
}
#endif
#endif
