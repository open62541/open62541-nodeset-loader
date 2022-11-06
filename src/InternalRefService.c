/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "InternalRefService.h"
#include "nodes/NodeContainer.h"
#include <NodesetLoader/NodesetLoader.h>
#include <stdlib.h>

struct InternalRefService
{
    size_t hierachicalRefsSize;
    NL_ReferenceTypeNode *hierachicalRefs;
    struct NodeContainer *nonHierachicalRefs;
};

typedef struct InternalRefService InternalRefService;

#define MAX_HIERACHICAL_REFS 50
NL_ReferenceTypeNode hierachicalRefs[MAX_HIERACHICAL_REFS] = {
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {35}},
     {0, "Organizes"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL,
    },
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {36}},
     {0, "HasEventSource"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {48}},
     {0, "HasNotifier"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {44}},
     {0, "Aggregates"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {45}},
     {0, "HasSubtype"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {47}},
     {0, "HasComponent"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {46}},
     {0, "HasProperty"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {38}},
     {0, "HasEncoding"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {33}},
     {0, "HierarchicalReferences"},
     {NULL, NULL},
     {NULL, NULL},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {NULL, NULL},
     NULL},
};

static bool
isNonHierachicalRef(const InternalRefService *service,
                    const NL_Reference *ref) {
    // TODO: nonHierachicalrefs should also be imported first
    // we state that we know all references from namespace 0
    if (ref->refType.namespaceIndex == 0)
        return true;

    for (size_t i = 0; i < service->nonHierachicalRefs->size; i++) {
        if (UA_NodeId_equal(&ref->refType,
                            &service->nonHierachicalRefs->nodes[i]->id))
            return true;
    }
    return false;
}

static bool
isHierachicalReference(const InternalRefService *service,
                       const NL_Reference *ref) {
    for (size_t i = 0; i < service->hierachicalRefsSize; i++) {
        if (UA_NodeId_equal(&ref->refType, &service->hierachicalRefs[i].id))
            return true;
    }
    return false;
}

static bool
isTypeDefRef(const InternalRefService* service, const NL_Reference* ref) {
    UA_NodeId hasTypeDefId = UA_NODEID_NUMERIC(0, 40);
    return UA_NodeId_equal(&ref->refType, &hasTypeDefId);
}

static void
addnewRefType(InternalRefService *service, NL_ReferenceTypeNode *node) {
    NL_Reference *ref = node->hierachicalRefs;
    bool isHierachical = false;
    while (ref) {
        if (!ref->isForward) {
            for (size_t i = 0; i < service->hierachicalRefsSize; i++) {
                if (UA_NodeId_equal(&service->hierachicalRefs[i].id, &ref->target)) {
                    service->hierachicalRefs[service->hierachicalRefsSize++] =
                        *(NL_ReferenceTypeNode *)node;
                    isHierachical = true;
                    break;
                }
            }
        }
        ref = ref->next;
    }
    if (!isHierachical) {
        NodeContainer_add(service->nonHierachicalRefs, (NL_Node *)node);
    }
}

NL_ReferenceService *InternalRefService_new()
{
    InternalRefService *service =
        (InternalRefService *)calloc(1, sizeof(InternalRefService));
    if(!service)
    {
        return NULL;
    }
    service->hierachicalRefs = hierachicalRefs;
    service->hierachicalRefsSize = 9;
    service->nonHierachicalRefs = NodeContainer_new(100, false);

    NL_ReferenceService *refService = (NL_ReferenceService *)calloc(1, sizeof(NL_ReferenceService));
    if(!refService)
    {
        free(service);
        return NULL;
    }
    refService->context = service;
    refService->addNewReferenceType =
        (RefService_addNewReferenceType)addnewRefType;
    refService->isHierachicalRef =
        (RefService_isHierachicalRef)isHierachicalReference;
    refService->isNonHierachicalRef =
        (RefService_isNonHierachicalRef)isNonHierachicalRef;
    refService->isHasTypeDefRef = (RefService_isHasTypeDefRef)isTypeDefRef;
    return refService;
}

void InternalRefService_delete(NL_ReferenceService *refService)
{
    InternalRefService *internalService =
        (InternalRefService *)refService->context;
    NodeContainer_delete(internalService->nonHierachicalRefs);
    free(internalService);
    free(refService);
}
