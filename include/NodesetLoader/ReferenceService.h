#ifndef NODESETLOADER_REFERENCESERVICE_H
#define NODESETLOADER_REFERENCESERVICE_H
#include <stdbool.h>

struct Reference;
struct TReferenceTypeNode;
typedef bool (*RefService_isHierachicalRef)(void* context, const struct Reference* ref);
typedef bool (*RefService_isNonHierachicalRef)(void* context, const struct Reference *ref);
typedef bool (*RefService_isHasTypeDefRef)(void *context, const struct Reference *ref);
typedef void (*RefService_addNewReferenceType)(void* context, const struct TReferenceTypeNode* node);
struct RefService
{
    void* context;
    RefService_isHierachicalRef isHierachicalRef;
    RefService_isNonHierachicalRef isNonHierachicalRef;
    RefService_isHasTypeDefRef isHasTypeDefRef;
    RefService_addNewReferenceType addNewReferenceType;
};
typedef struct RefService RefService;
#endif
