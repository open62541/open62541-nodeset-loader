/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NODESETLOADER_INTERNAL_H
#define NODESETLOADER_INTERNAL_H

#include <open62541/server.h>

#include <stdbool.h>
#include <stddef.h>

#define NL_NODECLASS_COUNT 8
typedef enum {
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5,
    NODECLASS_VARIABLETYPE = 6,
    NODECLASS_VIEW = 7
} NL_NodeClass;

typedef struct NL_Node NL_Node;

typedef struct NL_Reference {
    bool isForward;
    UA_NodeId refType;
    UA_NodeId target;
    NL_Node *targetPtr;
    struct NL_Reference *next;
} NL_Reference;

#define NL_NODE_ATTRIBUTES                                              \
    NL_NodeClass nodeClass;                                             \
    UA_NodeId id;                                                       \
    UA_QualifiedName browseName;                                        \
    UA_LocalizedText displayName;                                       \
    UA_LocalizedText description;                                       \
    NL_Reference *refs;                                                 \
    bool isSorted;                                                      \
    struct {                                                            \
        NL_Node *left;                                                  \
        NL_Node *right;                                                 \
    } treeEntry;                                                        \
    NL_Node *sortNext;

struct NL_Node { NL_NODE_ATTRIBUTES };

typedef struct { NL_NODE_ATTRIBUTES char *eventNotifier; } NL_ObjectNode;
typedef struct { NL_NODE_ATTRIBUTES char *isAbstract; } NL_ObjectTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    char *isAbstract;
    UA_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
} NL_VariableTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
    char *accessLevel;
    char *userAccessLevel;
    char *historizing;
    char *minimumSamplingInterval;
    UA_String value;
} NL_VariableNode;
typedef struct {
    char *name;
    UA_NodeId dataType;
    int valueRank;
    int value;
    bool isOptional;
} NL_DataTypeDefinitionField;
typedef struct {
    NL_DataTypeDefinitionField *fields;
    size_t fieldCnt;
    bool isEnum;
    bool isUnion;
    bool isOptionSet;
} NL_DataTypeDefinition;
typedef struct {
    NL_NODE_ATTRIBUTES
    NL_DataTypeDefinition *definition;
    char *isAbstract;
} NL_DataTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    char *executable;
    char *userExecutable;
} NL_MethodNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_LocalizedText inverseName;
    char *symmetric;
} NL_ReferenceTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    char *containsNoLoops;
    char *eventNotifier;
} NL_ViewNode;

typedef struct {
    UA_Server *server;
    UA_NamespaceMapping namespaceMapping;
    UA_Logger *logger;
    size_t parentRefTypesSize;
    UA_ExpandedNodeId *parentRefTypes;
} UA_NodeSetLoaderContext;

UA_StatusCode
UA_NodeSetLoaderContext_init(UA_NodeSetLoaderContext *context,
                             UA_Server *server, UA_Logger *logger);
void
UA_NodeSetLoaderContext_clear(UA_NodeSetLoaderContext *context);
UA_StatusCode
UA_NodeSetLoaderContext_addNamespace(UA_NodeSetLoaderContext *context,
                                     UA_String namespaceUri,
                                     bool localOnly);

typedef struct UA_NodeSetLoader UA_NodeSetLoader;
UA_NodeSetLoader *
UA_NodeSetLoader_new(UA_NodeSetLoaderContext *context);
UA_StatusCode
UA_NodeSetLoader_import(UA_NodeSetLoader *loader, const UA_ByteString *xml);
bool
UA_NodeSetLoader_sort(UA_NodeSetLoader *loader);
void
UA_NodeSetLoader_delete(UA_NodeSetLoader *loader);
typedef bool (*UA_NodeSetLoaderVisitor)(void *context, NL_Node *node);
bool
UA_NodeSetLoader_forEach(UA_NodeSetLoader *loader, void *context,
                         UA_NodeSetLoaderVisitor visitor);

UA_NodeId
UA_NodeSetLoader_getParentId(const UA_NodeSetLoaderContext *context,
                             const NL_Node *node, UA_NodeId *parentRefId);
UA_StatusCode
UA_NodeSetLoader_addCustomDataType(UA_NodeSetLoaderContext *context,
                                   const NL_DataTypeNode *node);

#endif
