/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef SORT_H
#define SORT_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TNode;
struct Nodeset;
struct SortContext;
typedef struct SortContext SortContext;
SortContext* Sort_init(void);
void Sort_cleanup(SortContext * ctx);
void Sort_addNode(SortContext* ctx, struct TNode *node);
typedef void (*Sort_SortedNodeCallback)(struct Nodeset *nodeset, struct TNode *node);
bool Sort_start(SortContext* ctx, struct Nodeset *nodeset, Sort_SortedNodeCallback callback);

#ifdef __cplusplus
}
#endif

#endif
