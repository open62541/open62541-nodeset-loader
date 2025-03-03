/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef __BACKEND_H__
#define __BACKEND_H__

#include "NodesetLoader/NodesetLoader.h"
#include <stdbool.h>
#include <stdio.h>

unsigned short _addNamespace(void *userContext, const char *namespaceUri);
void dumpNode(void *userContext, const NL_Node *node);

#endif
