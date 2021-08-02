/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <NodesetLoader/NodeId.h>
#include <string.h>

int TNodeId_cmp(const NL_NodeId *id1, const NL_NodeId *id2)
{
    if (id1->nsIdx == id2->nsIdx)
    {
        return strcmp(id1->id, id2->id);
    }
    if (id1->nsIdx < id2->nsIdx)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}
