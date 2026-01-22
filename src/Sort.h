/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef SORT_H
#define SORT_H

#include <stdbool.h>
#include "NodesetLoader/NodesetLoader.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NL_Node;
struct Nodeset;
struct SortContext;
typedef struct SortContext SortContext;
SortContext* Sort_init(void);
void Sort_cleanup(SortContext * ctx);
bool Sort_addNode(SortContext* ctx, struct NL_Node *node);
typedef void (*Sort_SortedNodeCallback)(struct Nodeset *nodeset, struct NL_Node *node);
bool Sort_start(SortContext* ctx, struct Nodeset *nodeset, Sort_SortedNodeCallback callback, struct UA_Logger *logger);

#ifdef __cplusplus
}
#endif

#endif
