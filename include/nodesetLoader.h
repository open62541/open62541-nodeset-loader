#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <stdbool.h>

#define NODECLASS_COUNT 6
typedef enum {
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5
    // TODO: variabletype, eventtype missing
} TNodeClass;

typedef struct {
    int nsIdx;
    const char *id;
    const char *idString;
} TNodeId;

struct Reference;
typedef struct Reference Reference;

struct Reference {
    bool isForward;
    TNodeId refType;
    TNodeId target;
    Reference *next;
};

#define UA_NODE_ATTRIBUTES                                                               \
    TNodeClass nodeClass;                                                                \
    TNodeId id;                                                                          \
    const char *browseName;                                                              \
    const char *displayName;                                                             \
    const char *description;                                                             \
    const char *writeMask;                                                               \
    Reference *hierachicalRefs;                                                          \
    Reference *nonHierachicalRefs;

typedef struct { UA_NODE_ATTRIBUTES } TNode;

typedef struct {
    UA_NODE_ATTRIBUTES
    TNodeId parentNodeId;
    const char *eventNotifier;
} TObjectNode;

typedef struct {
    UA_NODE_ATTRIBUTES
    const char *isAbstract;
} TObjectTypeNode;

typedef struct {
    UA_NODE_ATTRIBUTES
    TNodeId parentNodeId;
    TNodeId datatype;
    const char *arrayDimensions;
    const char *valueRank;
} TVariableNode;

typedef struct TDataTypeNode { UA_NODE_ATTRIBUTES } TDataTypeNode;

typedef struct {
    UA_NODE_ATTRIBUTES
    TNodeId parentNodeId;
} TMethodNode;

typedef struct { UA_NODE_ATTRIBUTES } TReferenceTypeNode;

typedef void (*addNodeCb)(const TNode *);

typedef int (*addNamespaceCb)(const char *);

typedef struct {
    int loadTimeMs;
    int sortTimeMs;
    int addNodeTimeMs;
} Statistics;

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    addNodeCb callback;
    const Statistics *stat;
} FileHandler;

void loadFile(const FileHandler *fileHandler);

#endif