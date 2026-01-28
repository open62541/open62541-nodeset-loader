/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "../../../src/Node.h"
#include "NodesetLoader/NodesetLoader.h"

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
     {0, UA_STRING_STATIC("Organizes")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL,
    },
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {36}},
     {0, UA_STRING_STATIC("HasEventSource")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {48}},
     {0, UA_STRING_STATIC("HasNotifier")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {44}},
     {0, UA_STRING_STATIC("Aggregates")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {45}},
     {0, UA_STRING_STATIC("HasSubtype")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {47}},
     {0, UA_STRING_STATIC("HasComponent")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {46}},
     {0, UA_STRING_STATIC("HasProperty")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {38}},
     {0, UA_STRING_STATIC("HasEncoding")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
    {NODECLASS_REFERENCETYPE,
     {0, UA_NODEIDTYPE_NUMERIC, {33}},
     {0, UA_STRING_STATIC("HierarchicalReferences")},
     {{0, NULL}, {0, NULL}},
     {{0, NULL}, {0, NULL}},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     false,
     {{0, NULL}, {0, NULL}},
     NULL},
};

static bool
isRefNonHierachical(const InternalRefService *service,
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
isReferenceHierachical(const InternalRefService *service,
                       const NL_Reference *ref) {
    for (size_t i = 0; i < service->hierachicalRefsSize; i++) {
        if (UA_NodeId_equal(&ref->refType, &service->hierachicalRefs[i].id))
            return true;
    }
    return false;
}

static bool
isRefTypeDef(const InternalRefService* service, const NL_Reference* ref) {
    UA_NodeId hasTypeDefId = UA_NODEID_NUMERIC(0, 40);
    return UA_NodeId_equal(&ref->refType, &hasTypeDefId);
}

static void
addnewRefTypeImpl(InternalRefService *service, NL_ReferenceTypeNode *node) {
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

NL_ReferenceService *RefService_new(void)
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
        (RefService_addNewReferenceType)addnewRefTypeImpl;
    refService->isHierachicalRef =
        (RefService_isRefHierachical)isReferenceHierachical;
    refService->isNonHierachicalRef =
        (RefService_isRefNonHierachical)isRefNonHierachical;
    refService->isHasTypeDefRef = (RefService_isHasTypeDefRef)isRefTypeDef;
    return refService;
}

void RefService_delete(NL_ReferenceService *refService)
{
    InternalRefService *internalService =
        (InternalRefService *)refService->context;
    NodeContainer_delete(internalService->nonHierachicalRefs);
    free(internalService);
    free(refService);
}
