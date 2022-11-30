/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_REFERENCESERVICE_H
#define NODESETLOADER_REFERENCESERVICE_H
#include <stdbool.h>

struct NL_Reference;
struct NL_ReferenceTypeNode;
typedef bool (*RefService_isRefHierachical)(void* context, const struct NL_Reference* ref);
typedef bool (*RefService_isRefNonHierachical)(void* context, const struct NL_Reference *ref);
typedef bool (*RefService_isHasTypeDefRef)(void *context, const struct NL_Reference *ref);
typedef void (*RefService_addNewReferenceType)(void* context, const struct NL_ReferenceTypeNode* node);
struct NL_ReferenceService
{
    void* context;
    RefService_isRefHierachical isHierachicalRef;
    RefService_isRefNonHierachical isNonHierachicalRef;
    RefService_isHasTypeDefRef isHasTypeDefRef;
    RefService_addNewReferenceType addNewReferenceType;
};
typedef struct NL_ReferenceService NL_ReferenceService;
#endif
