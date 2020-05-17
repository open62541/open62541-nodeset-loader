#include "InternalRefService.h"
#include "nodes/NodeContainer.h"
#include <NodesetLoader/NodesetLoader.h>

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
     {NULL, NULL},
     NULL},
};

static bool isNonHierachicalRef(Nodeset *nodeset, const Reference *ref)
{
    // TODO: nonHierachicalrefs should also be imported first
    // we state that we know all references from namespace 0
    if (ref->refType.nsIdx == 0)
    {
        return true;
    }
    for (size_t i = 0; i < nodeset->nonHierachicalRefs->size; i++)
    {
        if (!TNodeId_cmp(&ref->refType,
                         &nodeset->nonHierachicalRefs->nodes[i]->id))
        {
            return true;
        }
    }
    return false;
}

static bool isHierachicalReference(Nodeset *nodeset, const Reference *ref)
{
    for (size_t i = 0; i < nodeset->hierachicalRefsSize; i++)
    {
        if (!TNodeId_cmp(&ref->refType, &nodeset->hierachicalRefs[i].id))
        {
            return true;
        }
    }
    return false;
}

static void addToRefTypes(Nodeset *nodeset, TNode *node)
{
    Reference *ref = node->hierachicalRefs;
    bool isHierachical = false;
    while (ref)
    {
        if (!ref->isForward)
        {
            for (size_t i = 0; i < nodeset->hierachicalRefsSize; i++)
            {
                if (!TNodeId_cmp(&nodeset->hierachicalRefs[i].id, &ref->target))
                {
                    nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] =
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
        NodeContainer_add(nodeset->nonHierachicalRefs, node);
    }
}

struct RefService *InternalReferenceService_new()
{
    InternalRefService *service =
        (InternalRefService *)calloc(1, sizeof(InternalRefService));
    assert(service);
    service->hierachicalRefs = hierachicalRefs;
    service->hierachicalRefsSize = 9;
    service->nonHierachicalRefs = NodeContainer_new(100, false);

    RefService* refService = (RefService*)calloc(1, sizeof(RefService));
    assert(refService);
}