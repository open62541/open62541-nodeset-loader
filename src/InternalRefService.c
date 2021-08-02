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
    TReferenceTypeNode *hierachicalRefs;
    struct NodeContainer *nonHierachicalRefs;
};

typedef struct InternalRefService InternalRefService;

#define MAX_HIERACHICAL_REFS 50
TReferenceTypeNode hierachicalRefs[MAX_HIERACHICAL_REFS] = {
    {
        NODECLASS_REFERENCETYPE,
        {0, "i=35"},
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
     {0, "i=36"},
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
     {0, "i=48"},
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
     {0, "i=44"},
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
     {0, "i=45"},
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
     {0, "i=47"},
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
     {0, "i=46"},
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
     {0, "i=47"},
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
     {0, "i=33"},
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
};

static bool isNonHierachicalRef(const InternalRefService *service,
                                const Reference *ref)
{
    // TODO: nonHierachicalrefs should also be imported first
    // we state that we know all references from namespace 0
    if (ref->refType.nsIdx == 0)
    {
        return true;
    }
    for (size_t i = 0; i < service->nonHierachicalRefs->size; i++)
    {
        if (!TNodeId_cmp(&ref->refType,
                         &service->nonHierachicalRefs->nodes[i]->id))
        {
            return true;
        }
    }
    return false;
}

static bool isHierachicalReference(const InternalRefService *service,
                                   const Reference *ref)
{
    for (size_t i = 0; i < service->hierachicalRefsSize; i++)
    {
        if (!TNodeId_cmp(&ref->refType, &service->hierachicalRefs[i].id))
        {
            return true;
        }
    }
    return false;
}

static bool isTypeDefRef(const InternalRefService* service, const Reference* ref)
{
    TNodeId hasTypeDefId = {0, "i=40"};
    return !(TNodeId_cmp(&ref->refType, &hasTypeDefId));
}

static void addnewRefType(InternalRefService *service, TReferenceTypeNode *node)
{
    Reference *ref = node->hierachicalRefs;
    bool isHierachical = false;
    while (ref)
    {
        if (!ref->isForward)
        {
            for (size_t i = 0; i < service->hierachicalRefsSize; i++)
            {
                if (!TNodeId_cmp(&service->hierachicalRefs[i].id, &ref->target))
                {
                    service->hierachicalRefs[service->hierachicalRefsSize++] =
                        *(TReferenceTypeNode *)node;
                    isHierachical = true;
                    break;
                }
            }
        }
        ref = ref->next;
    }
    if (!isHierachical)
    {
        NodeContainer_add(service->nonHierachicalRefs, (NL_Node *)node);
    }
}

RefService *InternalRefService_new()
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

    RefService *refService = (RefService *)calloc(1, sizeof(RefService));
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

void InternalRefService_delete(RefService *refService)
{
    InternalRefService *internalService =
        (InternalRefService *)refService->context;
    NodeContainer_delete(internalService->nonHierachicalRefs);
    free(internalService);
    free(refService);
}
