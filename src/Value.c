#include "Value.h"
#include <stdlib.h>

Value *Value_new(const TNode *node)
{
    Value *newValue = (Value *)calloc(1, sizeof(Value));
    newValue->ctx = (ParserCtx *)calloc(1, sizeof(ParserCtx));
    newValue->ctx->state = PARSERSTATE_INIT;
    return newValue;
}

void Value_start(Value *val, const char *name)
{
    switch (val->ctx->state)
    {
    case PARSERSTATE_INIT:
        if (!strncmp(name, "ListOf", strlen("ListOf")))
        {
            val->ctx->state = PARSERSTATE_LISTOF;
            val->isArray= true;
            val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
            val->ctx->currentData->type = DATATYPE_PRIMITIVE;
            val->ctx->currentData->name = name;
        }
        else if (!strcmp(name, "ExtensionObject"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
        }
        else
        {
            val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
            val->ctx->currentData->type = DATATYPE_PRIMITIVE;
            val->ctx->currentData->name = name;
            val->data = val->ctx->currentData;
            val->ctx->state = PARSERSTATE_DATA;
        }       
        
        break;

    case PARSERSTATE_LISTOF:
        if (!strcmp(name, "ExtensionObject"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
            val->isExtensionObject = true;
            val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
            val->ctx->currentData->type = DATATYPE_PRIMITIVE;
            val->ctx->currentData->name = name;
            break;
        }
        val->ctx->state = PARSERSTATE_DATA;
        if (!val->ctx->currentData)
        {
            val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
            val->ctx->currentData->type = DATATYPE_PRIMITIVE;
            val->ctx->currentData->name = name;
        }
        else
        {
            Data *currentData = val->ctx->currentData;
            currentData->type = DATATYPE_COMPLEX;
            currentData->val.complexData.members = (Data **)realloc(
                currentData->val.complexData.members,
                (currentData->val.complexData.membersSize + 1) *
                    sizeof(Data *));

            Data *newData = (Data *)calloc(1, sizeof(Data));
            currentData->val.complexData
                .members[currentData->val.complexData.membersSize] = newData;

            currentData->val.complexData.membersSize++;
            newData->type = DATATYPE_PRIMITIVE;
            newData->name = name;
            newData->parent = currentData;
            val->ctx->currentData = newData;
        }
        break;

    case PARSERSTATE_EXTENSIONOBJECT:
        if(!strcmp(name, "TypeId"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_TYPEID;
            break;
        }
        if(!strcmp(name, "Body"))
        {
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_BODY;
        }
        break;
    case PARSERSTATE_EXTENSIONOBJECT_BODY:
        val->ctx->state = PARSERSTATE_DATA;
        val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
        val->ctx->currentData->type = DATATYPE_COMPLEX;
        val->ctx->currentData->name = name;
        val->data = val->ctx->currentData;
        break;

    case PARSERSTATE_EXTENSIONOBJECT_TYPEID:
        break;

    case PARSERSTATE_DATA:
        if (!val->ctx->currentData)
        {
            val->ctx->currentData = (Data *)calloc(1, sizeof(Data));
            val->ctx->currentData->type = DATATYPE_PRIMITIVE;
            val->ctx->currentData->name = name;
        }
        else
        {
            Data *currentData = val->ctx->currentData;
            currentData->type = DATATYPE_COMPLEX;
            currentData->val.complexData.members = (Data **)realloc(
                currentData->val.complexData.members,
                (currentData->val.complexData.membersSize + 1) *
                    sizeof(Data *));

            Data *newData = (Data *)calloc(1, sizeof(Data));
            currentData->val.complexData
                .members[currentData->val.complexData.membersSize] = newData;
            
            currentData->val.complexData.membersSize++;
            newData->type = DATATYPE_PRIMITIVE;
            newData->name = name;
            newData->parent = currentData;
            val->ctx->currentData = newData;
        }

        break;
    case PARSERSTATE_FINISHED:
        break;
    }
}

void Value_end(Value * val, const char *name, const char *value)
{
    switch(val->ctx->state)
    {
        case PARSERSTATE_INIT:
            val->ctx->state = PARSERSTATE_FINISHED;
            break;

        case PARSERSTATE_EXTENSIONOBJECT_TYPEID:
            if(!strcmp(name, "TypeId"))
            {
                val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
            }
            break;
            

        case PARSERSTATE_EXTENSIONOBJECT:
            if(!strcmp(name, "ExtensionObject"))
            {
                val->ctx->state = PARSERSTATE_INIT;
            }
            break;

        case PARSERSTATE_DATA:
            if(val->ctx->currentData->parent == NULL && !strcmp(name, val->ctx->currentData->name))
            {
                if(!val->isExtensionObject)
                {
                    val->ctx->state = PARSERSTATE_INIT;
                }
                else
                {
                    val->ctx->state = PARSERSTATE_EXTENSIONOBJECT_BODY;
                }
            }
            if (val->ctx->currentData->type == DATATYPE_PRIMITIVE)
            {
                val->ctx->currentData->val.primitiveData.value = value;
            }
            val->ctx->currentData = val->ctx->currentData->parent;
            break;

        case PARSERSTATE_EXTENSIONOBJECT_BODY:
            val->ctx->state = PARSERSTATE_EXTENSIONOBJECT;
            break;
        case PARSERSTATE_FINISHED:
        case PARSERSTATE_LISTOF:
            break;
    }
}