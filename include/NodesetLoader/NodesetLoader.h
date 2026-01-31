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

LOADER_EXPORT extern const char *NL_NODECLASS_NAME[NL_NODECLASS_COUNT];

struct NL_Node;
typedef struct NL_Node NL_Node;

struct NL_Reference;
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
    char *writeMask;                                                    \
    NL_Reference *refs;                                                 \
    void *extension;                                                    \
    bool isDone; /* the node was successfully added in the backend */

struct NL_Node {
    NL_NODE_ATTRIBUTES
};

typedef struct NL_InstanceNode {
    NL_NODE_ATTRIBUTES
} NL_InstanceNode;

typedef struct NL_ObjectNode {
    NL_NODE_ATTRIBUTES
    char *eventNotifier;
} NL_ObjectNode;

typedef struct NL_ObjectTypeNode {
    NL_NODE_ATTRIBUTES
    char *isAbstract;
} NL_ObjectTypeNode;

typedef struct NL_VariableTypeNode {
    NL_NODE_ATTRIBUTES
    char *isAbstract;
    UA_NodeId datatype;
    char *arrayDimensions;
    char *valueRank;
} NL_VariableTypeNode;

typedef struct NL_VariableNode {
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

typedef struct NL_DataTypeDefinitionField {
    char *name;
    UA_NodeId dataType;
    int valueRank;
    int value;
    bool isOptional;
} NL_DataTypeDefinitionField;

typedef struct NL_DataTypeDefinition {
    NL_DataTypeDefinitionField *fields;
    size_t fieldCnt;
    bool isEnum;
    bool isUnion;
    bool isOptionSet;
} NL_DataTypeDefinition;

typedef struct NL_DataTypeNode {
    NL_NODE_ATTRIBUTES
    NL_DataTypeDefinition *definition;
    char *isAbstract;
} NL_DataTypeNode;

typedef struct NL_MethodNode {
    NL_NODE_ATTRIBUTES
    char *executable;
    char *userExecutable;
} NL_MethodNode;

typedef struct NL_ReferenceTypeNode {
    NL_NODE_ATTRIBUTES
    UA_LocalizedText inverseName;
    char *symmetric;
} NL_ReferenceTypeNode;

typedef struct NL_ViewNode {
    NL_NODE_ATTRIBUTES
    char *containsNoLoops;
    char *eventNotifier;
} NL_ViewNode;

typedef void (*NL_addNamespaceCallback)(void *userContext,
                                        size_t localNamespaceUrisSize,
                                        UA_String *localNamespaceUris,
                                        UA_NamespaceMapping *nsMapping);

typedef struct NL_FileContext {
    void *userContext;
    const char *file;
    NL_addNamespaceCallback addNamespace;
    NodesetLoader_ExtensionInterface *extensionHandling;
    UA_NamespaceMapping *nsMapping;
} NL_FileContext;

struct NodesetLoader;
typedef struct NodesetLoader NodesetLoader;

LOADER_EXPORT NodesetLoader *
NodesetLoader_new(NodesetLoader_Logger *logger);

LOADER_EXPORT bool
NodesetLoader_importFile(NodesetLoader *loader,
                         const NL_FileContext *fileContext);

LOADER_EXPORT void
NodesetLoader_delete(NodesetLoader *loader);

LOADER_EXPORT bool
NodesetLoader_sort(NodesetLoader *loader);

typedef bool (*NodesetLoader_forEachNode_Func)(void *context, NL_Node *node);

// Returns false in case of an error
LOADER_EXPORT bool
NodesetLoader_forEachNode(NodesetLoader *loader, void *context,
                          NodesetLoader_forEachNode_Func fn);

#ifdef __cplusplus
}
#endif
#endif
