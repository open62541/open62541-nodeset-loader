/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"

int addNamespace(void* userContext, const char* uri)
{
    return 1;
}

void addNode(void* userContext, const TNode* node)
{
    return;
}

struct Value *Value_new(const TNode *node)
{
    return NULL;
}

void Value_start(Value *val, const char *localname)
{

}
void Value_end(Value *val, const char *localname, char *value)
{

}
void Value_finish(Value *val)
{

}
void Value_delete(Value **val)
{
    
}