/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef SORT_H
#define SORT_H
#include <stdbool.h>

struct TNode;
struct Nodeset;
void init(void);
void addNodeToSort(struct TNode *node);
typedef void (*OnSortCallback)(struct Nodeset* nodeset, struct TNode *node);
bool sort(struct Nodeset* nodeset, OnSortCallback callback);

#endif