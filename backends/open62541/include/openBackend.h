/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef __BACKEND_H__
#define __BACKEND_H__
#include <nodesetLoader/nodesetLoader.h>
#include <stdbool.h>
#include <stdio.h>

int addNamespace(void *userContext, const char *namespaceUri);
void addNode(void *userContext, const TNode *node);

struct Value *Value_new(const TNode *node);
void Value_start(Value *val, const char *localname);
void Value_end(Value *val, const char *localname, char *value);
void Value_finish(Value *val);
void Value_delete(Value **val);
#endif