/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"

static int namespaceIdx = 0;
int _addNamespace(void *userContext, const char *uri) { return namespaceIdx++; }

void dumpNode(void *userContext, const NL_Node *node)
{
    FILE *f = (FILE *)userContext;
    fprintf(f, "%d:%s,%s\n", node->id.nsIdx, node->id.id,
            node->browseName.name);
}
