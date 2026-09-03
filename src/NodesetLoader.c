/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "Nodeset.h"
#include <stdlib.h>
#include <string.h>

#include <yxml.h>

struct UA_NodeSetLoader
{
    Nodeset *nodeset;
};

typedef enum
{
    XML_TOKENIZE_OK,
    XML_TOKENIZE_INVALID,
    XML_TOKENIZE_OVERFLOW
} XmlTokenizeStatus;

typedef struct
{
    XmlTokenizeStatus status;
    size_t tokensSize;
} XmlTokenizeResult;

typedef struct
{
    char *data;
    size_t size;
    size_t capacity;
} XmlTextBuffer;

#define XML_TOKEN_STACK_SIZE 128
#define XML_YXML_STACK_SIZE 4096

static bool XmlTextBuffer_append(XmlTextBuffer *buf, const char *data,
                                 size_t length)
{
    if (buf->size > buf->capacity || length > buf->capacity - buf->size)
        return false;
    memcpy(buf->data + buf->size, data, length);
    buf->size += length;
    return true;
}

static bool XmlTextBuffer_appendName(XmlTextBuffer *buf, const char *name,
                                     size_t length, size_t *offset)
{
    *offset = buf->size;
    return XmlTextBuffer_append(buf, name, length) &&
           XmlTextBuffer_append(buf, "", 1);
}

static XmlTokenizeResult XmlTokenize(const char *xml, size_t xmlLength,
                                     XmlToken *tokens, size_t maxTokens,
                                     XmlTextBuffer *text)
{
    XmlTokenizeResult result;
    memset(&result, 0, sizeof(result));

    text->size = 0;
    if (!XmlTextBuffer_append(text, "", 1))
    {
        result.status = XML_TOKENIZE_INVALID;
        return result;
    }

    yxml_t parser;
    char parserStack[XML_YXML_STACK_SIZE];
    yxml_init(&parser, parserStack, sizeof(parserStack));

    XmlToken scratch[XML_TOKEN_STACK_SIZE];
    XmlToken *stack[XML_TOKEN_STACK_SIZE];
    bool stored[XML_TOKEN_STACK_SIZE];
    bool hasChildren[XML_TOKEN_STACK_SIZE];
    memset(scratch, 0, sizeof(scratch));
    memset(stored, 0, sizeof(stored));
    memset(hasChildren, 0, sizeof(hasChildren));

    XmlToken attributeScratch;
    XmlToken *attribute = NULL;
    bool attributeStored = false;
    size_t depth = 0;
    size_t tokenPosition = 0;

    for (size_t pos = 0; pos < xmlLength; pos++)
    {
        yxml_ret_t status = yxml_parse(&parser, (unsigned char)xml[pos]);
        if (status < YXML_OK)
        {
            result.status = XML_TOKENIZE_INVALID;
            result.tokensSize = tokenPosition;
            return result;
        }

        switch (status)
        {
        case YXML_OK:
        case YXML_PISTART:
        case YXML_PICONTENT:
        case YXML_PIEND:
            break;

        case YXML_ELEMSTART: {
            if (depth >= XML_TOKEN_STACK_SIZE)
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            if (depth > 0)
            {
                hasChildren[depth - 1] = true;
                stack[depth - 1]->contentLength = 0;
            }

            stored[depth] = (tokenPosition < maxTokens);
            XmlToken *token =
                stored[depth] ? &tokens[tokenPosition] : &scratch[depth];
            memset(token, 0, sizeof(*token));
            token->type = XML_TOKEN_ELEMENT;
            size_t nameLength = yxml_symlen(&parser, parser.elem);
            if (stored[depth] &&
                !XmlTextBuffer_appendName(text, parser.elem, nameLength,
                                          &token->name))
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            if (nameLength < pos + 1)
                token->start = pos - nameLength - 1;
            stack[depth] = token;
            hasChildren[depth] = false;
            depth++;
            tokenPosition++;
            break;
        }

        case YXML_ATTRSTART: {
            if (depth == 0)
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            stack[depth - 1]->attributes++;
            attributeStored = (tokenPosition < maxTokens);
            attribute =
                attributeStored ? &tokens[tokenPosition] : &attributeScratch;
            memset(attribute, 0, sizeof(*attribute));
            attribute->type = XML_TOKEN_ATTRIBUTE;
            size_t nameLength = yxml_symlen(&parser, parser.attr);
            if (attributeStored &&
                !XmlTextBuffer_appendName(text, parser.attr, nameLength,
                                          &attribute->name))
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            tokenPosition++;
            break;
        }

        case YXML_CONTENT:
            if (depth > 0 && stored[depth - 1] && !hasChildren[depth - 1])
            {
                size_t length = strlen(parser.data);
                if (stack[depth - 1]->contentLength == 0)
                    stack[depth - 1]->content = text->size;
                if (!XmlTextBuffer_append(text, parser.data, length))
                {
                    result.status = XML_TOKENIZE_INVALID;
                    result.tokensSize = tokenPosition;
                    return result;
                }
                stack[depth - 1]->contentLength += length;
            }
            break;

        case YXML_ATTRVAL:
            if (attributeStored)
            {
                size_t length = strlen(parser.data);
                if (attribute->contentLength == 0)
                    attribute->content = text->size;
                if (!XmlTextBuffer_append(text, parser.data, length))
                {
                    result.status = XML_TOKENIZE_INVALID;
                    result.tokensSize = tokenPosition;
                    return result;
                }
                attribute->contentLength += length;
            }
            break;

        case YXML_ATTREND:
            if (attributeStored && attribute->contentLength > 0 &&
                !XmlTextBuffer_append(text, "", 1))
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            attribute = NULL;
            attributeStored = false;
            break;

        case YXML_ELEMEND:
            if (depth == 0)
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            depth--;
            stack[depth]->end = pos + 1;
            stack[depth]->subtreeEnd = tokenPosition;
            if (stored[depth] && stack[depth]->contentLength > 0 &&
                !XmlTextBuffer_append(text, "", 1))
            {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            break;

        default:
            result.status = XML_TOKENIZE_INVALID;
            result.tokensSize = tokenPosition;
            return result;
        }
    }

    result.tokensSize = tokenPosition;
    if (yxml_eof(&parser) != YXML_OK || depth != 0)
    {
        result.status = XML_TOKENIZE_INVALID;
    }
    else if (tokenPosition > maxTokens)
    {
        result.status = XML_TOKENIZE_OVERFLOW;
    }
    return result;
}

static const char *localName(const char *qualifiedName)
{
    const char *colon = strrchr(qualifiedName, ':');
    return colon ? colon + 1 : qualifiedName;
}

typedef enum
{
    XML_SCOPE_DOCUMENT,
    XML_SCOPE_NODE,
    XML_SCOPE_NAMESPACE_URIS,
    XML_SCOPE_REFERENCES,
    XML_SCOPE_DEFINITION
} XmlScope;

typedef struct
{
    const XmlToken *tokens;
    size_t tokensSize;
    size_t position;
    char *text;
    const char *xml;
    size_t xmlLength;
} XmlCursor;

static bool XmlToken_nodeClass(const char *name, NL_NodeClass *nodeClass)
{
    static const char *const names[NL_NODECLASS_COUNT] = {
        "UAObject", "UAObjectType",    "UAVariable",     "UADataType",
        "UAMethod", "UAReferenceType", "UAVariableType", "UAView"};
    for (size_t i = 0; i < NL_NODECLASS_COUNT; i++)
    {
        if (strcmp(name, names[i]))
            continue;
        *nodeClass = (NL_NodeClass)i;
        return true;
    }
    return false;
}

static char *XmlToken_leafContent(XmlCursor *cursor, const XmlToken *element)
{
    cursor->position = element->subtreeEnd;
    if (element->contentLength == 0)
        return NULL;
    return cursor->text + element->content;
}

static bool ProcessElement(Nodeset *nodeset, XmlCursor *cursor, XmlScope scope,
                           NL_Node *node);

static bool ProcessChildren(Nodeset *nodeset, XmlCursor *cursor,
                            const XmlToken *element, XmlScope scope,
                            NL_Node *node)
{
    if (element->subtreeEnd < cursor->position ||
        element->subtreeEnd > cursor->tokensSize)
        return false;
    while (cursor->position < element->subtreeEnd)
    {
        if (!ProcessElement(nodeset, cursor, scope, node))
            return false;
    }
    return cursor->position == element->subtreeEnd;
}

static bool ProcessElement(Nodeset *nodeset, XmlCursor *cursor, XmlScope scope,
                           NL_Node *node)
{
    if (cursor->position >= cursor->tokensSize)
        return false;

    const XmlToken *element = &cursor->tokens[cursor->position++];
    if (element->type != XML_TOKEN_ELEMENT ||
        element->subtreeEnd < cursor->position ||
        element->subtreeEnd > cursor->tokensSize)
        return false;

    size_t attributePosition = cursor->position;
    if (element->attributes > element->subtreeEnd - cursor->position)
        return false;
    for (size_t i = 0; i < element->attributes; i++)
    {
        if (cursor->tokens[attributePosition + i].type != XML_TOKEN_ATTRIBUTE)
            return false;
    }
    cursor->position += element->attributes;

    const char *name = localName(cursor->text + element->name);
    XmlAttributes attributes = {&cursor->tokens[attributePosition],
                                element->attributes, cursor->text};

    if (scope == XML_SCOPE_DOCUMENT)
    {
        NL_NodeClass nodeClass;
        if (XmlToken_nodeClass(name, &nodeClass))
        {
            NL_Node *newNode =
                UA_NodeSet_newNode(nodeset, nodeClass, &attributes);
            if (!newNode)
                return false;
            return ProcessChildren(nodeset, cursor, element, XML_SCOPE_NODE,
                                   newNode);
        }
        else if (!strcmp(name, "NamespaceUris"))
        {
            return ProcessChildren(nodeset, cursor, element,
                                   XML_SCOPE_NAMESPACE_URIS, NULL);
        }
        else if (!strcmp(name, "Alias"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            return UA_NodeSet_addAlias(nodeset, &attributes, content);
        }
        else if (!strcmp(name, "UANodeSet") || !strcmp(name, "Aliases"))
        {
            return ProcessChildren(nodeset, cursor, element, XML_SCOPE_DOCUMENT,
                                   NULL);
        }
    }
    else if (scope == XML_SCOPE_NODE)
    {
        if (!strcmp(name, "DisplayName"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            UA_NodeSet_setLocalizedText(&node->displayName, &attributes,
                                        content);
            return true;
        }
        else if (!strcmp(name, "References"))
        {
            return ProcessChildren(nodeset, cursor, element,
                                   XML_SCOPE_REFERENCES, node);
        }
        else if (!strcmp(name, "Description"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            UA_NodeSet_setLocalizedText(&node->description, &attributes,
                                        content);
            return true;
        }
        else if (!strcmp(name, "Value"))
        {
            cursor->position = element->subtreeEnd;
            if (node->nodeClass != NODECLASS_VARIABLE)
                return true;
            if (element->end < element->start ||
                element->end > cursor->xmlLength)
                return false;
            UA_String xmlValue = {
                element->end - element->start,
                (UA_Byte *)(uintptr_t)(cursor->xml + element->start)};
            return UA_String_copy(&xmlValue,
                                  &((NL_VariableNode *)node)->value) ==
                   UA_STATUSCODE_GOOD;
        }
        else if (!strcmp(name, "Definition") &&
                 node->nodeClass == NODECLASS_DATATYPE)
        {
            if (!UA_NodeSet_addDataTypeDefinition(node, &attributes))
                return false;
            return ProcessChildren(nodeset, cursor, element,
                                   XML_SCOPE_DEFINITION, node);
        }
        else if (!strcmp(name, "InverseName"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            if (node->nodeClass == NODECLASS_REFERENCETYPE)
                UA_NodeSet_setLocalizedText(
                    &((NL_ReferenceTypeNode *)node)->inverseName, &attributes,
                    content);
            return true;
        }
    }
    else if (scope == XML_SCOPE_NAMESPACE_URIS)
    {
        if (!strcmp(name, "Uri"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            return UA_NodeSet_addNamespace(nodeset, content);
        }
    }
    else if (scope == XML_SCOPE_REFERENCES)
    {
        if (!strcmp(name, "Reference"))
        {
            char *content = XmlToken_leafContent(cursor, element);
            return UA_NodeSet_addReference(nodeset, node, &attributes, content);
        }
    }
    else if (scope == XML_SCOPE_DEFINITION)
    {
        if (!strcmp(name, "Field"))
        {
            if (!UA_NodeSet_addDataTypeField(nodeset, node, &attributes))
                return false;
            cursor->position = element->subtreeEnd;
            return true;
        }
    }

    /* Unknown elements are irrelevant together with their entire subtree. */
    cursor->position = element->subtreeEnd;
    return true;
}

static UA_StatusCode Parser_run(Nodeset *nodeset, const UA_ByteString *xml)
{
    if (!xml || (!xml->data && xml->length > 0) || xml->length == SIZE_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    XmlToken tokenBuffer[64];
    XmlToken *tokens = tokenBuffer;
    size_t tokensCapacity = 64;
    /* Decoded names and leaf values cannot exceed their source XML size. */
    XmlTextBuffer text = {(char *)UA_malloc(xml->length + 1), 0,
                          xml->length + 1};
    if (!text.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    XmlTokenizeResult result = XmlTokenize((const char *)xml->data, xml->length,
                                           tokens, tokensCapacity, &text);
    if (result.status == XML_TOKENIZE_OVERFLOW)
    {
        tokensCapacity = result.tokensSize;
        tokens = NULL;
        if (tokensCapacity <= SIZE_MAX / sizeof(XmlToken))
            tokens = (XmlToken *)UA_malloc(tokensCapacity * sizeof(XmlToken));
        if (tokens)
            result = XmlTokenize((const char *)xml->data, xml->length, tokens,
                                 tokensCapacity, &text);
    }

    bool textOwned = false;
    UA_StatusCode status = UA_STATUSCODE_BADDECODINGERROR;
    if (tokens && result.status == XML_TOKENIZE_OK && result.tokensSize > 0)
    {
        XmlCursor cursor = {tokens,    result.tokensSize,       0,
                            text.data, (const char *)xml->data, xml->length};
        if (UA_NodeSet_ownTextBuffer(nodeset, text.data))
        {
            textOwned = true;
        }
        else
        {
            status = UA_STATUSCODE_BADOUTOFMEMORY;
        }
        if (textOwned &&
            ProcessElement(nodeset, &cursor, XML_SCOPE_DOCUMENT, NULL) &&
            cursor.position == cursor.tokensSize)
            status = UA_STATUSCODE_GOOD;
    }
    else if (!tokens)
    {
        status = UA_STATUSCODE_BADOUTOFMEMORY;
    }

    if (tokens != tokenBuffer)
        UA_free(tokens);
    if (!textOwned)
        UA_free(text.data);
    return status;
}

UA_StatusCode UA_NodeSetLoader_import(UA_NodeSetLoader *loader,
                                      const UA_ByteString *xml)
{
    if (!loader || !loader->nodeset)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return Parser_run(loader->nodeset, xml);
}

bool UA_NodeSetLoader_sort(UA_NodeSetLoader *loader)
{
    if (!loader)
        return false;
    return UA_NodeSet_sort(loader->nodeset);
}

UA_NodeSetLoader *UA_NodeSetLoader_new(UA_NodeSetLoaderContext *context)
{
    if (!context || !context->logger)
        return NULL;

    UA_NodeSetLoader *loader =
        (UA_NodeSetLoader *)UA_calloc(1, sizeof(UA_NodeSetLoader));
    if (!loader)
        return NULL;
    loader->nodeset = UA_NodeSet_new(context);
    if (!loader->nodeset)
    {
        UA_free(loader);
        return NULL;
    }
    return loader;
}

void UA_NodeSetLoader_delete(UA_NodeSetLoader *loader)
{
    if (!loader)
        return;
    UA_NodeSet_cleanup(loader->nodeset);
    UA_free(loader);
}

bool UA_NodeSetLoader_forEach(UA_NodeSetLoader *loader, void *context,
                              UA_NodeSetLoaderVisitor fn)
{
    if (!loader || !loader->nodeset || !fn)
        return false;
    for (NL_Node *node = loader->nodeset->sorted.head; node;
         node = node->sortNext)
    {
        if (!fn(context, node))
            return false;
    }
    return true;
}
