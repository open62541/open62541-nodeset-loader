/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NAMESPACELIST_H
#define NAMESPACELIST_H
#include "NodesetLoader/NodesetLoader.h"

struct NamespaceList;
typedef struct NamespaceList NamespaceList;

struct Namespace;
typedef struct Namespace Namespace;

struct Namespace
{
    short unsigned idx;
    const char *name;
};

NamespaceList *NamespaceList_new(NL_addNamespaceCallback cb);
Namespace *NamespaceList_newNamespace(NamespaceList *list, void *userContext,
                                      const char *uri);
void NamespaceList_setUri(NamespaceList *list, Namespace *ns);
void NamespaceList_delete(NamespaceList *list);
const Namespace *NamespaceList_getNamespace(const NamespaceList *list,
                                            int relativeIndex);

#endif
