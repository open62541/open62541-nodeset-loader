/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_TNODEID_H
#define NODESETLOADER_TNODEID_H
#include "arch.h"
typedef struct
{
    int nsIdx;
    char *id;
} NL_NodeId;
LOADER_EXPORT int TNodeId_cmp(const NL_NodeId *id1, const NL_NodeId *id2);
#endif
