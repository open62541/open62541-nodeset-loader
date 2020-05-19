/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "Value.h"
#include <stdlib.h>

Value *Value_new(const TNode *node)
{
    Value *newValue = (Value *)calloc(1, sizeof(Value));
    newValue->ctx = (ParserCtx *)calloc(1, sizeof(ParserCtx));
    newValue->ctx->state = PARSERSTATE_INIT;
    return newValue;
}

static Data *newData(const char *name, DataType type)
{
    Data *newData = (Data *)calloc(1, sizeof(Data));
    newData->type = type;
    newData->name = name;
    return newData;
}

static Data *addNewMember(Data *parent, const char *name)
{
    parent->type = DATATYPE_COMPLEX;
    parent->val.complexData.members = (Data **)realloc(
        parent->val.complexData.members,
        (parent->val.complexData.membersSize + 1) * sizeof(Data *));

    Data *newData = (Data *)calloc(1, sizeof(Data));
    parent->val.complexData.members[parent->val.complexData.membersSize] =
        newData;

    parent->val.complexData.membersSize++;
    newData->type = DATATYPE_PRIMITIVE;
    newData->name = name;
    newData->parent = parent;
    return newData;
}

void Value_start(Value *val, const char *name)
{
    switch (val->ctx->state)
    {
    case PARSERSTATE_INIT:
        if (!strncmp(name, "ListOf", strlen("ListOf")))
        {
            val->ctx->state = PARSERSTATE_LISTOF;
            val->isArray = true;
            val->data = newData(name, DATATYPE_COMPLEX);
            val->ctx->currentData = val->data;
        }
        else if (!strcmp(name, "ExtensionObject"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
            val->isExtensionObject = true;
        }
        else
        {
            val->type = name;
            val->data = newData(name, DATATYPE_PRIMITIVE);
            val->ctx->currentData = val->data;
            val->ctx->state = PARSERSTATE_DATA;
        }
        break;

    case PARSERSTATE_LISTOF:
        if (!strcmp(name, "ExtensionObject"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
            val->isExtensionObject = true;
            break;
        }
        val->ctx->state = PARSERSTATE_DATA;
        {
            val->type = name;
            Data *newData = addNewMember(val->ctx->currentData, name);
            val->ctx->currentData = newData;
        }

        break;

    case PARSERSTATE_EXTENSIONOBJECT:
        if (!strcmp(name, "TypeId"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_TYPEID;
            break;
        }
        if (!strcmp(name, "Body"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_BODY;
        }
        break;
    case PARSERSTATE_EXTENSIONOBJECT_BODY:
        val->ctx->state = PARSERSTATE_DATA;
        if (!val->ctx->currentData)
        {
            val->data = newData(name, DATATYPE_COMPLEX);
            val->ctx->currentData = val->data;
        }
        else
        {
            Data *newData = addNewMember(val->ctx->currentData, name);
            val->ctx->currentData = newData;
        }
        break;

    case PARSERSTATE_EXTENSIONOBJECT_TYPEID:
        break;

    case PARSERSTATE_DATA:
        if (!val->ctx->currentData)
        {
            val->data = newData(name, DATATYPE_PRIMITIVE);
            val->ctx->currentData = val->data;
        }
        else
        {
            Data *newData = addNewMember(val->ctx->currentData, name);
            val->ctx->currentData = newData;
        }

        break;
    case PARSERSTATE_FINISHED:
        break;
    }
}

void Value_end(Value *val, const char *name, const char *value)
{
    switch (val->ctx->state)
    {
    case PARSERSTATE_INIT:
        val->ctx->state = PARSERSTATE_FINISHED;
        break;

    case PARSERSTATE_EXTENSIONOBJECT_TYPEID:
        if (!strcmp(name, "Identifier"))
        {
            val->type = value;
        }
        if (!strcmp(name, "TypeId"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
        }
        break;

    case PARSERSTATE_EXTENSIONOBJECT:
        if (!strcmp(name, "ExtensionObject"))
        {
            val->ctx->state = PARSERSTATE_INIT;
        }
        break;

    case PARSERSTATE_DATA:
        if (strcmp(name, val->ctx->currentData->name))
        {
            break;
        }
        if (val->ctx->currentData->type == DATATYPE_PRIMITIVE)
        {
            val->ctx->currentData->val.primitiveData.value = value;
        }
        val->ctx->currentData = val->ctx->currentData->parent;
        // exit conditions
        if (!val->isExtensionObject && val->ctx->currentData == NULL)
        {
            val->ctx->state = PARSERSTATE_INIT;
        }
        // extensionsobject
        if (val->isExtensionObject && !val->isArray &&
            val->ctx->currentData == NULL)
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_BODY;
        }
        // listOfExtensionObject
        if (val->isExtensionObject && val->isArray &&
            val->ctx->currentData->parent == NULL)
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_BODY;
        }
        break;

    case PARSERSTATE_EXTENSIONOBJECT_BODY:
        val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
        break;
    case PARSERSTATE_FINISHED:
    case PARSERSTATE_LISTOF:
        break;
    }
}
static void Data_clear(Data *data);

static void PrimitiveData_clear(Data *data)
{
    // free(data);
}

static void ComplexData_clear(Data *data)
{
    for (size_t cnt = 0; cnt < data->val.complexData.membersSize; cnt++)
    {
        Data_clear(data->val.complexData.members[cnt]);
    }
    free(data->val.complexData.members);
}

static void Data_clear(Data *data)
{
    if (data->type == DATATYPE_PRIMITIVE)
    {
        PrimitiveData_clear(data);
    }
    else
    {
        ComplexData_clear(data);
    }
    free(data);
}

void Value_delete(Value *val)
{
    Data_clear(val->data);
    free(val->ctx);
    free(val);
}
