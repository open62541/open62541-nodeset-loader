/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef ALIASLIST_H
#define ALIASLIST_H

#include <NodesetLoader/NodesetLoader.h>

struct Alias
{
    char *name;
    UA_NodeId id;
};

typedef struct Alias Alias;

struct AliasList;
typedef struct AliasList AliasList;
AliasList *AliasList_new(void);
Alias *AliasList_newAlias(AliasList *list, char *name);
const UA_NodeId *AliasList_getNodeId(const AliasList *list, const char *alias);
void AliasList_delete(AliasList *list);

#endif
