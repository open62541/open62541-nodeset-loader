/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "InternalRefService.h"
#include "Nodeset.h"
#include "Parser.h"
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
#define INVERSENAME "InverseName"

const char *NL_NODECLASS_NAME[NL_NODECLASS_COUNT] = {
    "Object", "ObjectType",    "Variable",    "DataType",
    "Method", "ReferenceType", "VariableType", "View"};

struct NodesetLoader {
    Nodeset *nodeset;
    UA_Logger *logger;
    NL_ReferenceService *refService;
    bool internalRefService;
};

static void OnStartElementNs(void *ctx, const char *localname,
                             const char *prefix, const char *URI,
                             int nb_namespaces, const char **namespaces,
                             int nb_attributes, int nb_defaulted,
                             const char **attributes) {
    TParserCtx *pctx = (TParserCtx *)ctx;

    /* We are below an unknown element */
    if(pctx->unknown_depth > 0) {
        pctx->unknown_depth++;
        return;
    }

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
            pctx->alias = Nodeset_newAlias(pctx->nodeset, nb_attributes, attributes);
            pctx->state = PARSER_STATE_ALIAS;
        }
        else if (!strcmp(localname, "UANodeSet") ||
                 !strcmp(localname, "Aliases"))
        {
            pctx->state = PARSER_STATE_INIT;
        }
        else
        {
            pctx->unknown_depth++;
            return;
        }
        break;
    case PARSER_STATE_NAMESPACEURIS:
        if (!strcmp(localname, NAMESPACEURI)) {
            pctx->state = PARSER_STATE_URI;
        } else {
            pctx->unknown_depth++;
            return;
        }
        break;
    case PARSER_STATE_URI:
        pctx->unknown_depth++;
        return;
    case PARSER_STATE_NODE:
        if (!strcmp(localname, DISPLAYNAME)) {
            Nodeset_setDisplayName(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
            pctx->state = PARSER_STATE_DISPLAYNAME;
        } else if (!strcmp(localname, REFERENCES)) {
            pctx->state = PARSER_STATE_REFERENCES;
        } else if (!strcmp(localname, DESCRIPTION)) {
            pctx->state = PARSER_STATE_DESCRIPTION;
            Nodeset_setDescription(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
        } else if (!strcmp(localname, VALUE)) {
            pctx->state = PARSER_STATE_VALUE;
            pctx->value_depth++;
            pctx->valueBegin = pctx->ctxt->input->cur - pctx->ctxt->input->base;
            while(pctx->buf[pctx->valueBegin] != '<')
                pctx->valueBegin--;
        } else if (!strcmp(localname, "Definition")) {
            pctx->state = PARSER_STATE_DATATYPE_DEFINITION;
        } else if (!strcmp(localname, INVERSENAME)) {
            pctx->state = PARSER_STATE_INVERSENAME;
            Nodeset_setInverseName(pctx->nodeset, pctx->node, nb_attributes,
                                   attributes);
        } else {
            pctx->unknown_depth++;
            return;
        }
        break;

    case PARSER_STATE_VALUE:
        if(!strcmp(localname, VALUE))
            pctx->value_depth++; /* Nested <Value> elements */
        break;

    case PARSER_STATE_REFERENCES:
        if (!strcmp(localname, REFERENCE)) {
            pctx->state = PARSER_STATE_REFERENCE;
            pctx->ref = Nodeset_newReference(pctx->nodeset, pctx->node,
                                             nb_attributes, attributes);
        } else {
            pctx->unknown_depth++;
            return;
        }
        break;
    case PARSER_STATE_DATATYPE_DEFINITION:
    case PARSER_STATE_DESCRIPTION:
    case PARSER_STATE_ALIAS:
    case PARSER_STATE_DISPLAYNAME:
    case PARSER_STATE_REFERENCE:
    case PARSER_STATE_INVERSENAME:
        pctx->unknown_depth++;
        return;
    }
    pctx->onCharacters = NULL;
    pctx->onCharLength = 0;
}

static void
OnEndElementNs(void *ctx, const char *localname,
               const char *prefix, const char *URI) {
    TParserCtx *pctx = (TParserCtx *)ctx;

    if(pctx->unknown_depth > 0) {
        pctx->unknown_depth--;
        return;
    }

    switch (pctx->state)
    {
    case PARSER_STATE_INIT:
        break;
    case PARSER_STATE_ALIAS:
        Nodeset_newAliasFinish(pctx->nodeset, pctx->alias, pctx->onCharacters);
        pctx->state = PARSER_STATE_INIT;
        break;
    case PARSER_STATE_URI:
        Nodeset_newNamespaceFinish(pctx->nodeset, pctx->userContext, pctx->onCharacters);
        pctx->state = PARSER_STATE_NAMESPACEURIS;
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
        Nodeset_newReferenceFinish(pctx->nodeset, pctx->ref, pctx->node, pctx->onCharacters);
        pctx->state = PARSER_STATE_REFERENCES;
        break;
    case PARSER_STATE_VALUE:
        if(!strcmp(localname, VALUE)) {
            pctx->value_depth--;
            if(pctx->value_depth == 0) {
                /* Leaving the value element. Store the value */
                /* TODO: Enable VariableType to hold a valeu */
                if(pctx->node->nodeClass == NODECLASS_VARIABLE) {
                    long valueEnd = pctx->ctxt->input->cur - pctx->ctxt->input->base;
                    UA_String xmlValue;
                    xmlValue.data = (UA_Byte*)pctx->buf + pctx->valueBegin;
                    xmlValue.length = (size_t)(valueEnd - pctx->valueBegin);
                    UA_String_copy(&xmlValue, &((NL_VariableNode *)pctx->node)->value);
                }
                pctx->state = PARSER_STATE_NODE;
            }
        }
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
    }
    pctx->onCharacters = NULL;
    pctx->onCharLength = 0;
}

static void OnCharacters(void *ctx, const char *ch, int len) {
    TParserCtx *pctx = (TParserCtx *)ctx;
    if (pctx->onCharacters == NULL) {
        pctx->onCharacters = CharArenaAllocator_malloc(pctx->nodeset->charArena, (size_t)len + 1);
    } else {
        pctx->onCharacters = CharArenaAllocator_realloc(
            pctx->nodeset->charArena, (size_t)len + 1);
    }
    memcpy(pctx->onCharacters + pctx->onCharLength, ch, (size_t)len);
    pctx->onCharLength += (size_t)len;
}

bool NodesetLoader_importFile(NodesetLoader *loader,
                              const NL_FileContext *fileHandler) {
    if(!fileHandler) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER, "NodesetLoader: no filehandler - abort");
        return false;
    }

    if(!fileHandler->addNamespace) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER, "NodesetLoader: fileHandler->addNamespace missing");
        return false;
    }

    if(!loader->nodeset) {
        loader->nodeset = Nodeset_new(fileHandler->addNamespace, loader->logger,
                                      loader->refService);
    }

    TParserCtx ctx;
    bool retStatus = true;
    FILE *f = fopen(fileHandler->file, "r");
    memset(&ctx, 0, sizeof(struct TParserCtx));

    if(!f) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER, "NodesetLoader: file open error");
        retStatus = false;
        goto cleanup;
    }

    ctx.nodeset = loader->nodeset;
    ctx.state = PARSER_STATE_INIT;
    ctx.userContext = fileHandler->userContext;
    ctx.nodeset = loader->nodeset;
    ctx.nodeset->fc = (NL_FileContext*)(uintptr_t)fileHandler;

    /* Initialize the nodeset context with ns0 */
    if(loader->nodeset->localNamespaceUrisSize == 0) {
        UA_String ns0 = UA_STRING("http://opcfoundation.org/UA/");
        UA_StatusCode ret =
            UA_Array_appendCopy((void**)&ctx.nodeset->localNamespaceUris,
                                &ctx.nodeset->localNamespaceUrisSize,
                                &ns0, &UA_TYPES[UA_TYPES_STRING]);
        (void)ret;

        loader->nodeset->fc->nsMapping.remote2local = (UA_UInt16*)
            UA_calloc(1, sizeof(UA_UInt16));
        loader->nodeset->fc->nsMapping.remote2localSize = 1;
    }

    if(Parser_run(&ctx, f, OnStartElementNs, OnEndElementNs, OnCharacters)) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER, "NodesetLoader: xml parsing error");
        retStatus = false;
    }

    /* Clean up values that were added on the fly */
    UA_Array_delete(loader->nodeset->localNamespaceUris,
                    loader->nodeset->localNamespaceUrisSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    loader->nodeset->localNamespaceUris = NULL;
    loader->nodeset->localNamespaceUrisSize = 0;

cleanup:
    if(f)
        fclose(f);
    return retStatus;
}

bool NodesetLoader_sort(NodesetLoader *loader) {
    return Nodeset_sort(loader->nodeset);
}

NodesetLoader *NodesetLoader_new(UA_Logger *logger,
                                 NL_ReferenceService *refService) {
    NodesetLoader *loader = (NodesetLoader *)calloc(1, sizeof(NodesetLoader));
    if(!loader)
        return NULL;

    loader->logger = logger;
    if(!refService) {
        loader->refService = InternalRefService_new();
        loader->internalRefService = true;
    } else {
        loader->refService = refService;
    }

    return loader;
}

void NodesetLoader_delete(NodesetLoader *loader) {
    Nodeset_cleanup(loader->nodeset);
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
