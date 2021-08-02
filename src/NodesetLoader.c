/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "InternalLogger.h"
#include "InternalRefService.h"
#include "Nodeset.h"
#include "Parser.h"
#include "Value.h"
#include <CharAllocator.h>
#include <NodesetLoader/Logger.h>
#include <NodesetLoader/NodesetLoader.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define OBJECT "UAObject"
#define METHOD "UAMethod"
#define OBJECTTYPE "UAObjectType"
#define VARIABLE "UAVariable"
#define VARIABLETYPE "UAVariableType"
#define DATATYPE "UADataType"
#define REFERENCETYPE "UAReferenceType"
#define VIEW "UAView"
#define DISPLAYNAME "DisplayName"
#define REFERENCES "References"
#define REFERENCE "Reference"
#define DESCRIPTION "Description"
#define ALIAS "Alias"
#define NAMESPACEURIS "NamespaceUris"
#define NAMESPACEURI "Uri"
#define VALUE "Value"
#define EXTENSIONS "Extensions"
#define EXTENSION "Extension"
#define INVERSENAME "InverseName"

const char *NL_NODECLASS_NAME[NODECLASS_COUNT] = {
    "Object", "ObjectType",    "Variable",    "DataType",
    "Method", "ReferenceType", "VariableType", "View"};

typedef enum
{
    PARSER_STATE_INIT,
    PARSER_STATE_NODE,
    PARSER_STATE_DISPLAYNAME,
    PARSER_STATE_REFERENCES,
    PARSER_STATE_REFERENCE,
    PARSER_STATE_DESCRIPTION,
    PARSER_STATE_INVERSENAME,
    PARSER_STATE_ALIAS,
    PARSER_STATE_UNKNOWN,
    PARSER_STATE_NAMESPACEURIS,
    PARSER_STATE_URI,
    PARSER_STATE_VALUE,
    PARSER_STATE_EXTENSION,
    PARSER_STATE_EXTENSIONS,
    PARSER_STATE_DATATYPE_DEFINITION,
    PARSER_STATE_DATATYPE_DEFINITION_FIELD
} TParserState;

struct TParserCtx
{
    void *userContext;
    TParserState state;
    TParserState prev_state;
    size_t unknown_depth;
    NL_NodeClass nodeClass;
    NL_Node *node;
    struct Alias *alias;
    char *onCharacters;
    size_t onCharLength;
    Value *val;
    void *extensionData;
    NodesetLoader_ExtensionInterface *extIf;
    Reference *ref;
    Nodeset *nodeset;
};

struct NodesetLoader
{
    Nodeset *nodeset;
    NodesetLoader_Logger *logger;
    bool internalLogger;
    RefService *refService;
    bool internalRefService;
};

static void enterUnknownState(TParserCtx *ctx)
{
    ctx->prev_state = ctx->state;
    ctx->state = PARSER_STATE_UNKNOWN;
    ctx->unknown_depth = 1;
}

static void OnStartElementNs(void *ctx, const char *localname,
                             const char *prefix, const char *URI,
                             int nb_namespaces, const char **namespaces,
                             int nb_attributes, int nb_defaulted,
                             const char **attributes)
{
    TParserCtx *pctx = (TParserCtx *)ctx;
    switch (pctx->state)
    {
    case PARSER_STATE_INIT:
        if (!strcmp(localname, VARIABLE))
        {
            pctx->nodeClass = NODECLASS_VARIABLE;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, OBJECT))
        {
            pctx->nodeClass = NODECLASS_OBJECT;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, OBJECTTYPE))
        {
            pctx->nodeClass = NODECLASS_OBJECTTYPE;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, DATATYPE))
        {
            pctx->nodeClass = NODECLASS_DATATYPE;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, METHOD))
        {
            pctx->nodeClass = NODECLASS_METHOD;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, REFERENCETYPE))
        {
            pctx->nodeClass = NODECLASS_REFERENCETYPE;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, VARIABLETYPE))
        {
            pctx->nodeClass = NODECLASS_VARIABLETYPE;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, VIEW))
        {
            pctx->nodeClass = NODECLASS_VIEW;
            pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                         nb_attributes, attributes);
            pctx->state = PARSER_STATE_NODE;
        }
        else if (!strcmp(localname, NAMESPACEURIS))
        {
            pctx->state = PARSER_STATE_NAMESPACEURIS;
        }
        else if (!strcmp(localname, ALIAS))
        {
            pctx->state = PARSER_STATE_ALIAS;
            pctx->node = NULL;
            pctx->alias =
                Nodeset_newAlias(pctx->nodeset, nb_attributes, attributes);
            pctx->state = PARSER_STATE_ALIAS;
        }
        else if (!strcmp(localname, "UANodeSet") ||
                 !strcmp(localname, "Aliases") ||
                 !strcmp(localname, "Extensions"))
        {
            pctx->state = PARSER_STATE_INIT;
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_NAMESPACEURIS:
        if (!strcmp(localname, NAMESPACEURI))
        {
            pctx->state = PARSER_STATE_URI;
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_URI:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_NODE:
        if (!strcmp(localname, DISPLAYNAME))
        {
            Nodeset_setDisplayName(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
            pctx->state = PARSER_STATE_DISPLAYNAME;
        }
        else if (!strcmp(localname, REFERENCES))
        {
            pctx->state = PARSER_STATE_REFERENCES;
        }
        else if (!strcmp(localname, DESCRIPTION))
        {
            pctx->state = PARSER_STATE_DESCRIPTION;
            Nodeset_setDescription(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
        }
        else if (!strcmp(localname, VALUE))
        {
            pctx->val = Value_new(pctx->node);
            pctx->state = PARSER_STATE_VALUE;
        }
        else if (!strcmp(localname, EXTENSIONS))
        {
            pctx->state = PARSER_STATE_EXTENSIONS;
        }
        else if (!strcmp(localname, "Definition"))
        {
            Nodeset_addDataTypeDefinition(pctx->nodeset, pctx->node, nb_attributes,
                                     attributes);
            pctx->state = PARSER_STATE_DATATYPE_DEFINITION;
        }
        else if (!strcmp(localname, INVERSENAME))
        {
            pctx->state = PARSER_STATE_INVERSENAME;
            Nodeset_setInverseName(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_DATATYPE_DEFINITION:
        if (!strcmp(localname, "Field"))
        {
            Nodeset_addDataTypeField(pctx->nodeset, pctx->node, nb_attributes,
                                     attributes);
            pctx->state = PARSER_STATE_DATATYPE_DEFINITION_FIELD;
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_DATATYPE_DEFINITION_FIELD:
        enterUnknownState(pctx);
        break;

    case PARSER_STATE_VALUE:
        // copy the name
        {
            size_t len = strlen(localname);
            char *localNameCopy =
                CharArenaAllocator_malloc(pctx->nodeset->charArena, len + 1);
            memcpy(localNameCopy, localname, len);
            Value_start(pctx->val, localNameCopy);
            pctx->unknown_depth++;
        }
        break;

    case PARSER_STATE_EXTENSIONS:
        if (!strcmp(localname, EXTENSION))
        {
            if (pctx->extIf)
            {
                pctx->extensionData = pctx->extIf->newExtension();
            }
            pctx->state = PARSER_STATE_EXTENSION;
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_EXTENSION:
        if (pctx->extIf)
        {
            pctx->extIf->start(pctx->extensionData, localname, nb_attributes, attributes);
        }
        break;

    case PARSER_STATE_REFERENCES:
        if (!strcmp(localname, REFERENCE))
        {
            pctx->state = PARSER_STATE_REFERENCE;
            pctx->ref = Nodeset_newReference(pctx->nodeset, pctx->node,
                                             nb_attributes, attributes);
        }
        else
        {
            enterUnknownState(pctx);
        }
        break;
    case PARSER_STATE_DESCRIPTION:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_ALIAS:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_DISPLAYNAME:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_REFERENCE:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_INVERSENAME:
        enterUnknownState(pctx);
        break;
    case PARSER_STATE_UNKNOWN:
        pctx->unknown_depth++;
        break;    
    }
    pctx->onCharacters = NULL;
    pctx->onCharLength = 0;
}

static void OnEndElementNs(void *ctx, const char *localname, const char *prefix,
                           const char *URI)
{
    TParserCtx *pctx = (TParserCtx *)ctx;
    switch (pctx->state)
    {
    case PARSER_STATE_INIT:
        break;
    case PARSER_STATE_ALIAS:
        Nodeset_newAliasFinish(pctx->nodeset, pctx->alias, pctx->onCharacters);
        pctx->state = PARSER_STATE_INIT;
        break;
    case PARSER_STATE_URI:
    {
        Nodeset_newNamespaceFinish(pctx->nodeset, pctx->userContext,
                                   pctx->onCharacters);
        pctx->state = PARSER_STATE_NAMESPACEURIS;
    }
    break;
    case PARSER_STATE_NAMESPACEURIS:
        pctx->state = PARSER_STATE_INIT;
        break;
    case PARSER_STATE_NODE:
        Nodeset_newNodeFinish(pctx->nodeset, pctx->node);
        pctx->state = PARSER_STATE_INIT;
        break;
    case PARSER_STATE_DISPLAYNAME:
        Nodeset_DisplayNameFinish(pctx->nodeset, pctx->node,
                                  pctx->onCharacters);
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_REFERENCES:
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_REFERENCE:
    {
        Nodeset_newReferenceFinish(pctx->nodeset, pctx->ref, pctx->node,
                                   pctx->onCharacters);
        pctx->state = PARSER_STATE_REFERENCES;
    }
    break;
    case PARSER_STATE_VALUE:
        if (!strcmp(localname, VALUE) && pctx->unknown_depth == 0)
        {
            ((NL_VariableNode *)pctx->node)->value = pctx->val;
            pctx->state = PARSER_STATE_NODE;
        }
        else
        {
            Value_end(pctx->val, localname, pctx->onCharacters);
            pctx->unknown_depth--;
        }
        break;
    case PARSER_STATE_EXTENSION:
        if (!strcmp(localname, EXTENSION))
        {
            if (pctx->extIf)
            {
                pctx->extIf->finish(pctx->extensionData);
                pctx->node->extension = pctx->extensionData;
            }
            pctx->state = PARSER_STATE_EXTENSIONS;
        }
        else
        {
            if (pctx->extIf)
            {
                pctx->extIf->end(pctx->extensionData, localname,
                                 pctx->onCharacters);
            }
        }
        break;
    case PARSER_STATE_EXTENSIONS:
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_DESCRIPTION:
        Nodeset_DescriptionFinish(pctx->nodeset, pctx->node,
                                  pctx->onCharacters);
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_INVERSENAME:
        Nodeset_InverseNameFinish(pctx->nodeset, pctx->node,
                                  pctx->onCharacters);
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_DATATYPE_DEFINITION:
        pctx->state = PARSER_STATE_NODE;
        break;
    case PARSER_STATE_DATATYPE_DEFINITION_FIELD:
        pctx->state = PARSER_STATE_DATATYPE_DEFINITION;
        break;
    case PARSER_STATE_UNKNOWN:
        pctx->unknown_depth--;
        if (pctx->unknown_depth == 0)
        {
            pctx->state = pctx->prev_state;
        }
    }
    pctx->onCharacters = NULL;
    pctx->onCharLength = 0;
}

static void OnCharacters(void *ctx, const char *ch, int len)
{
    TParserCtx *pctx = (TParserCtx *)ctx;
    if (pctx->onCharacters == NULL)
    {
        char *newValue = CharArenaAllocator_malloc(pctx->nodeset->charArena,
                                                   (size_t)len + 1);
        pctx->onCharacters = newValue;
    }
    else
    {
        pctx->onCharacters = CharArenaAllocator_realloc(
            pctx->nodeset->charArena, (size_t)len + 1);
    }
    memcpy(pctx->onCharacters + pctx->onCharLength, ch, (size_t)len);
    pctx->onCharLength += (size_t)len;
}

bool NodesetLoader_importFile(NodesetLoader *loader,
                              const FileContext *fileHandler)
{
    if (fileHandler == NULL)
    {
        loader->logger->log(loader->logger->context,
                            NODESETLOADER_LOGLEVEL_ERROR,
                            "NodesetLoader: no filehandler - abort");
        return false;
    }
    if (fileHandler->addNamespace == NULL)
    {
        loader->logger->log(loader->logger->context,
                            NODESETLOADER_LOGLEVEL_ERROR,
                            "NodesetLoader: fileHandler->addNamespace missing");
        return false;
    }
    bool status = true;
    if (!loader->nodeset)
    {
        loader->nodeset = Nodeset_new(fileHandler->addNamespace, loader->logger,
                                      loader->refService);
    }

    TParserCtx *ctx = NULL;
    FILE *f = fopen(fileHandler->file, "r");

    if (!f)
    {
        loader->logger->log(loader->logger->context,
                            NODESETLOADER_LOGLEVEL_ERROR,
                            "NodesetLoader: file open error");
        status = false;
        goto cleanup;
    }

    ctx = (TParserCtx *)calloc(1, sizeof(TParserCtx));
    if (!ctx)
    {
        status = false;
        goto cleanup;
    }
    ctx->nodeset = loader->nodeset;
    ctx->state = PARSER_STATE_INIT;
    ctx->prev_state = PARSER_STATE_INIT;
    ctx->unknown_depth = 0;
    ctx->onCharacters = NULL;
    ctx->onCharLength = 0;
    ctx->userContext = fileHandler->userContext;
    ctx->extIf = fileHandler->extensionHandling;

    Parser *parser = Parser_new(ctx);
    if (Parser_run(parser, f, OnStartElementNs, OnEndElementNs, OnCharacters))
    {
        loader->logger->log(loader->logger->context,
                            NODESETLOADER_LOGLEVEL_ERROR, "xml parsing error");
        status = false;
    }
    Parser_delete(parser);

cleanup:
    free(ctx);
    if (f)
    {
        fclose(f);
    }
    return status;
}

bool NodesetLoader_sort(NodesetLoader *loader)
{
    return Nodeset_sort(loader->nodeset);
}

NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger,
                                 RefService *refService)
{
    NodesetLoader *loader = (NodesetLoader *)calloc(1, sizeof(NodesetLoader));
    if(!loader)
    {
        return NULL;
    }
    if (!logger)
    {
        loader->logger = InternalLogger_new();
        loader->internalLogger = true;
    }
    else
    {
        loader->logger = logger;
    }
    if (!refService)
    {
        loader->refService = InternalRefService_new();
        loader->internalRefService = true;
    }
    else
    {
        loader->refService = refService;
    }
    return loader;
}

void NodesetLoader_delete(NodesetLoader *loader)
{
    Nodeset_cleanup(loader->nodeset);
    if (loader->internalLogger)
    {
        free(loader->logger);
    }
    if (loader->internalRefService)
    {
        InternalRefService_delete(loader->refService);
    }
    free(loader);
}

const NL_BiDirectionalReference *
NodesetLoader_getBidirectionalRefs(const NodesetLoader *loader)
{
    return Nodeset_getBiDirectionalRefs(loader->nodeset);
}

size_t NodesetLoader_forEachNode(NodesetLoader *loader, NL_NodeClass nodeClass,
                               void *context,
                               NodesetLoader_forEachNode_Func fn)
{
    return Nodeset_forEachNode(loader->nodeset, nodeClass, context, fn);
}
