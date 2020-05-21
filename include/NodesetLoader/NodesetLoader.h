/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_NODESETLOADER_H
#define NODESETLOADER_NODESETLOADER_H
#include "Logger.h"
#include "TNodeId.h"
#include "arch.h"
#include "ReferenceService.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NODECLASS_COUNT 7
typedef enum
{
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5,
    NODECLASS_VARIABLETYPE = 6,
    // eventtype is handled like a object type
} TNodeClass;

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
    char* locale;
    char* text;
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
    Reference *unknownRefs;

struct TNode
{
    NODE_ATTRIBUTES
};
typedef struct TNode TNode;

struct TObjectNode
{
    NODE_ATTRIBUTES
    TNodeId parentNodeId;
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
    TNodeId parentNodeId;
    TNodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    char* historizing;
    Value *value;
    Reference* refToTypeDef;
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
} DataTypeDefinition;

struct TDataTypeNode
{
    NODE_ATTRIBUTES
    DataTypeDefinition *definition;
    char* isAbstract;
};
typedef struct TDataTypeNode TDataTypeNode;

struct TMethodNode
{
    NODE_ATTRIBUTES
    TNodeId parentNodeId;
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

typedef int (*addNamespaceCb)(void *userContext, const char *);



typedef void *(*newExtensionCb)(const TNode *);
typedef void (*startExtensionCb)(void *extensionData, const char *name);
typedef void (*endExtensionCb)(void *extensionData, const char *name,
                               char *value);
typedef void (*finishExtensionCb)(void *extensionData);

typedef struct
{
    void *userContext;
    newExtensionCb newExtension;
    startExtensionCb start;
    endExtensionCb end;
    finishExtensionCb finish;
} ExtensionInterface;

typedef struct
{
    void *userContext;
    const char *file;
    addNamespaceCb addNamespace;
    ExtensionInterface *extensionHandling;
} FileContext;

struct NodesetLoader;
typedef struct NodesetLoader NodesetLoader;

LOADER_EXPORT NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger, struct RefService* refService);
LOADER_EXPORT bool NodesetLoader_importFile(NodesetLoader *loader,
                                            const FileContext *fileContext);
LOADER_EXPORT void NodesetLoader_delete(NodesetLoader *loader);
LOADER_EXPORT size_t NodesetLoader_getNodes(const NodesetLoader *loader,
                                            TNodeClass nodeClass,
                                            TNode ***nodes);
LOADER_EXPORT const BiDirectionalReference *
NodesetLoader_getBidirectionalRefs(const NodesetLoader *loader);
LOADER_EXPORT bool NodesetLoader_sort(NodesetLoader *loader);

#ifdef __cplusplus
}
#endif
#endif
