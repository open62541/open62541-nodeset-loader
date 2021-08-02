/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include <NodesetLoader/NodesetLoader.h>
#include <NodesetLoader/NodeId.h>
#include <stdbool.h>
#include <stddef.h>

enum ParserState
{
    PARSERSTATE_INIT,
    PARSERSTATE_LISTOF,
    PARSERSTATE_EXTENSIONOBJECT,
    PARSERSTATE_EXTENSIONOBJECT_TYPEID,
    PARSERSTATE_EXTENSIONOBJECT_BODY,
    PARSERSTATE_DATA,
    PARSERSTATE_FINISHED
};
typedef enum ParserState ParserState;

struct NL_ParserCtx
{
    ParserState state;
    Data *currentData;
};
typedef struct NL_ParserCtx NL_ParserCtx;

NL_Value *Value_new(const NL_Node *node);
void Value_start(NL_Value *val, const char *name);
void Value_end(NL_Value *val, const char *name, const char *value);
void Value_delete(NL_Value *val);
