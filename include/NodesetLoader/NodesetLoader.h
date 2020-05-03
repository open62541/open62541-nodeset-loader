#pragma once
#include "TNodeId.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Logger.h"

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
    NODECLASS_VARIABLETYPE = 6
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
    BiDirectionalReference* next;
};

#define NODE_ATTRIBUTES                                                     \
    TNodeClass nodeClass;                                                      \
    TNodeId id;                                                                \
    TBrowseName browseName;                                                    \
    char *displayName;                                                         \
    char *description;                                                         \
    char *writeMask;                                                           \
    Reference *hierachicalRefs;                                                \
    Reference *nonHierachicalRefs;

struct TNode
{
    NODE_ATTRIBUTES
};
typedef struct TNode TNode;

typedef struct
{
    NODE_ATTRIBUTES
    TNodeId parentNodeId;
    char *eventNotifier;
} TObjectNode;

typedef struct
{
    NODE_ATTRIBUTES
    char *isAbstract;
} TObjectTypeNode;

typedef struct
{
    NODE_ATTRIBUTES
    char *isAbstract;
    TNodeId datatype;
    char *arrayDimensions;
    char *valueRank;
} TVariableTypeNode;

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
typedef struct
{
    NODE_ATTRIBUTES
    TNodeId parentNodeId;
    TNodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    Value *value;
} TVariableNode;

typedef struct
{
    char *name;
    TNodeId dataType;
    int valueRank;
    int value;
} DataTypeDefinitionField;

typedef struct
{
    DataTypeDefinitionField *fields;
    size_t fieldCnt;
    bool isEnum;
} DataTypeDefinition;

typedef struct TDataTypeNode
{
    NODE_ATTRIBUTES
    DataTypeDefinition *definition;
} TDataTypeNode;

typedef struct
{
    NODE_ATTRIBUTES
    TNodeId parentNodeId;
    char *executable;
    char *userExecutable;
} TMethodNode;

typedef struct
{
    NODE_ATTRIBUTES
    char *symmetric;
} TReferenceTypeNode;

typedef int (*addNamespaceCb)(void *userContext, const char *);



struct Extension;
typedef struct Extension Extension;

typedef Extension *(*newExtensionCb)(const TNode *);
typedef void (*startExtensionCb)(Extension *ext, const char *name);
typedef void (*endExtensionCb)(Extension *val, const char *name,
                               char *value);
typedef void (*finishExtensionCb)(Extension *val);

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

NodesetLoader *NodesetLoader_new(NodesetLoader_Logger* logger);
bool NodesetLoader_importFile(NodesetLoader *loader,
                              const FileContext *fileContext);
void NodesetLoader_delete(NodesetLoader *loader);
size_t NodesetLoader_getNodes(const NodesetLoader *loader, TNodeClass nodeClass,
                              TNode ***nodes);
const BiDirectionalReference* NodesetLoader_getBidirectionalRefs(const NodesetLoader* loader);
bool NodesetLoader_sort(NodesetLoader *loader);

#ifdef __cplusplus
}
#endif
