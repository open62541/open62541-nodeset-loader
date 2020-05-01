#ifndef NODESETLOADER_NODESETLOADER_H
#define NODESETLOADER_NODESETLOADER_H
#include "Logger.h"
#include "TNodeId.h"
#include "arch.h"
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
    BiDirectionalReference *next;
};

#define NODE_ATTRIBUTES                                                        \
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

/* Value Handling */
struct Value;
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

typedef Value *(*newValueCb)(const TNode *node);
typedef void (*startValueCb)(Value *val, const char *localname);
typedef void (*endValueCb)(Value *val, const char *localname, char *value);
typedef void (*finishValueCb)(Value *val);
typedef void (*deleteValueCb)(Value **val);

typedef void *(*newExtensionCb)(const TNode *);
typedef void (*startExtensionCb)(void *extensionData, const char *name);
typedef void (*endExtensionCb)(void *extensionData, const char *name,
                               char *value);
typedef void (*finishExtensionCb)(void *extensionData);

typedef struct
{
    void *userContext;
    newValueCb newValue;
    startValueCb start;
    endValueCb end;
    finishValueCb finish;
    deleteValueCb deleteValue;
} ValueInterface;

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
    ValueInterface *valueHandling;
    ExtensionInterface *extensionHandling;
} FileContext;

struct NodesetLoader;
typedef struct NodesetLoader NodesetLoader;

LOADER_EXPORT NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger);
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