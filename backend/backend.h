/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef __BACKEND_H__
#define __BACKEND_H__
#include <stdio.h>
#include <stdbool.h>
#include "xmlparser.h"

int addNamespace(const char *namespaceUri);
void addNode(const TNode *node);
#endif