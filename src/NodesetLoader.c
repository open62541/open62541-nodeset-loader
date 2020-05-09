/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "InternalLogger.h"
#include "Nodeset.h"
#include "Value.h"
#include <CharAllocator.h>
#include <NodesetLoader/Logger.h>
#include <NodesetLoader/NodesetLoader.h>
#include <assert.h>
#include <libxml/SAX.h>
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
    TNodeClass nodeClass;
    TNode *node;
    struct Alias *alias;
    char *onCharacters;
    size_t onCharLength;
    Value *val;
    void *extensionData;
    ExtensionInterface *extIf;
    Reference *ref;
    Nodeset *nodeset;
};

struct NodesetLoader
{
    Nodeset *nodeset;
    NodesetLoader_Logger *logger;
    bool internalLogger;
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
            pctx->state = PARSER_STATE_DATATYPE_DEFINITION;
        }
        else if (!strcmp(localname, INVERSENAME))
        {
            pctx->state = PARSER_STATE_INVERSENAME;
            Nodeset_setInverseName(pctx->nodeset, pctx->node, nb_attributes, attributes);
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
        }
        break;

    case PARSER_STATE_EXTENSIONS:
        if (!strcmp(localname, EXTENSION))
        {
            if (pctx->extIf)
            {
                pctx->extensionData = pctx->extIf->newExtension(pctx->node);
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
            pctx->extIf->start(pctx->extensionData, localname);
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
    case PARSER_STATE_UNKNOWN:
        pctx->unknown_depth++;
        break;
    case PARSER_STATE_INVERSENAME:
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
        if (!strcmp(localname, VALUE))
        {
            // Value_finish(pctx->val);
            ((TVariableNode *)pctx->node)->value = pctx->val;
            pctx->state = PARSER_STATE_NODE;
        }
        else
        {
            Value_end(pctx->val, localname, pctx->onCharacters);
        }
        break;
    case PARSER_STATE_EXTENSION:
        if (!strcmp(localname, EXTENSION))
        {
            if (pctx->extIf)
            {
                pctx->extIf->finish(pctx->extensionData);
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
        pctx->onCharacters = CharArenaAllocator_realloc(pctx->nodeset->charArena, (size_t)len + 1);
    }
    memcpy(pctx->onCharacters + pctx->onCharLength, ch, (size_t)len);
    pctx->onCharLength += (size_t)len;
}

static xmlSAXHandler make_sax_handler(void)
{
    xmlSAXHandler SAXHandler;
    memset(&SAXHandler, 0, sizeof(xmlSAXHandler));
    SAXHandler.initialized = XML_SAX2_MAGIC;
    // nodesets are encoded with UTF-8
    // this code does no transformation on the encoded text or interprets it
    // so it should be safe to cast xmlChar* to char*
    SAXHandler.startElementNs = (startElementNsSAX2Func)OnStartElementNs;
    SAXHandler.endElementNs = (endElementNsSAX2Func)OnEndElementNs;
    SAXHandler.characters = (charactersSAXFunc)OnCharacters;
    return SAXHandler;
}

static int read_xmlfile(FILE *f, TParserCtx *parserCtxt)
{
    char chars[1024];
    int res = (int)fread(chars, 1, 4, f);
    if (res <= 0)
    {
        return 1;
    }

    xmlSAXHandler SAXHander = make_sax_handler();
    xmlParserCtxtPtr ctxt =
        xmlCreatePushParserCtxt(&SAXHander, parserCtxt, chars, res, NULL);
    while ((res = (int)fread(chars, 1, sizeof(chars), f)) > 0)
    {
        if (xmlParseChunk(ctxt, chars, res, 0))
        {
            xmlParserError(ctxt, "xmlParseChunk");
            return 1;
        }
    }
    xmlParseChunk(ctxt, chars, 0, 1);
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    return 0;
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
        loader->nodeset = Nodeset_new(fileHandler->addNamespace, loader->logger);
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

    if (read_xmlfile(f, ctx))
    {
        loader->logger->log(loader->logger->context,
                            NODESETLOADER_LOGLEVEL_ERROR, "xml read error");
        status = false;
    }

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

NodesetLoader *NodesetLoader_new(NodesetLoader_Logger *logger)
{
    NodesetLoader *loader = (NodesetLoader *)calloc(1, sizeof(NodesetLoader));
    if (!logger)
    {
        loader->logger = InternalLogger_new();
        loader->internalLogger = true;
    }
    else
    {
        loader->logger = logger;
    }
    assert(loader);
    return loader;
}

void NodesetLoader_delete(NodesetLoader *loader)
{
    Nodeset_cleanup(loader->nodeset);
    if (loader->internalLogger)
    {
        free(loader->logger);
    }
    free(loader);
}

size_t NodesetLoader_getNodes(const NodesetLoader *loader, TNodeClass nodeClass,
                              TNode ***nodes)
{
    return Nodeset_getNodes(loader->nodeset, nodeClass, nodes);
}

const BiDirectionalReference *
NodesetLoader_getBidirectionalRefs(const NodesetLoader *loader)
{
    return Nodeset_getBiDirectionalRefs(loader->nodeset);
}
