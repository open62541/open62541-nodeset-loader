/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_REFERENCESERVICE_H
#define NODESETLOADER_REFERENCESERVICE_H
#include <stdbool.h>

struct Reference;
struct TReferenceTypeNode;
typedef bool (*RefService_isHierachicalRef)(void* context, const struct Reference* ref);
typedef bool (*RefService_isNonHierachicalRef)(void* context, const struct Reference *ref);
typedef void (*RefService_addNewReferenceType)(void* context, const struct TReferenceTypeNode* node);
struct RefService
{
    void* context;
    RefService_isHierachicalRef isHierachicalRef;
    RefService_isNonHierachicalRef isNonHierachicalRef;
    RefService_addNewReferenceType addNewReferenceType;
};
typedef struct RefService RefService;
#endif
