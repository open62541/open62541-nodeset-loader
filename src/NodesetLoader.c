/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "Nodeset.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <yxml.h>

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
    "Object", "ObjectType", "Variable", "DataType",
    "Method", "ReferenceType", "VariableType", "View"};

struct NodesetLoader {
    Nodeset *nodeset;
    UA_Logger *logger;
};

typedef enum {
    XML_TOKEN_ELEMENT,
    XML_TOKEN_ATTRIBUTE
} XmlTokenType;

typedef struct {
    XmlTokenType type;
    size_t name;
    size_t content;
    size_t contentLength;
    size_t attributes;
    size_t subtreeEnd;
    size_t start;
    size_t end;
} XmlToken;

typedef enum {
    XML_TOKENIZE_OK,
    XML_TOKENIZE_INVALID,
    XML_TOKENIZE_OVERFLOW
} XmlTokenizeStatus;

typedef struct {
    XmlTokenizeStatus status;
    size_t tokensSize;
} XmlTokenizeResult;

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} XmlTextBuffer;

#define XML_TOKEN_STACK_SIZE 128
#define XML_YXML_STACK_SIZE 4096

static bool
XmlTextBuffer_append(XmlTextBuffer *buf, const char *data, size_t length) {
    if(buf->size > buf->capacity || length > buf->capacity - buf->size)
        return false;
    memcpy(buf->data + buf->size, data, length);
    buf->size += length;
    return true;
}

static bool
XmlTextBuffer_appendName(XmlTextBuffer *buf, const char *name,
                         size_t length, size_t *offset) {
    *offset = buf->size;
    return XmlTextBuffer_append(buf, name, length) &&
        XmlTextBuffer_append(buf, "", 1);
}

static XmlTokenizeResult
XmlTokenize(const char *xml, size_t xmlLength, XmlToken *tokens,
            size_t maxTokens, XmlTextBuffer *text) {
    XmlTokenizeResult result;
    memset(&result, 0, sizeof(result));

    text->size = 0;
    if(!XmlTextBuffer_append(text, "", 1)) {
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

    for(size_t pos = 0; pos < xmlLength; pos++) {
        yxml_ret_t status = yxml_parse(&parser, (unsigned char)xml[pos]);
        if(status < YXML_OK) {
            result.status = XML_TOKENIZE_INVALID;
            result.tokensSize = tokenPosition;
            return result;
        }

        switch(status) {
        case YXML_OK:
        case YXML_PISTART:
        case YXML_PICONTENT:
        case YXML_PIEND:
            break;

        case YXML_ELEMSTART: {
            if(depth >= XML_TOKEN_STACK_SIZE) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            if(depth > 0) {
                hasChildren[depth - 1] = true;
                stack[depth - 1]->contentLength = 0;
            }

            stored[depth] = (tokenPosition < maxTokens);
            XmlToken *token = stored[depth] ? &tokens[tokenPosition] :
                &scratch[depth];
            memset(token, 0, sizeof(*token));
            token->type = XML_TOKEN_ELEMENT;
            size_t nameLength = yxml_symlen(&parser, parser.elem);
            if(stored[depth] &&
               !XmlTextBuffer_appendName(text, parser.elem, nameLength,
                                         &token->name)) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            if(nameLength < pos + 1)
                token->start = pos - nameLength - 1;
            stack[depth] = token;
            hasChildren[depth] = false;
            depth++;
            tokenPosition++;
            break;
        }

        case YXML_ATTRSTART: {
            if(depth == 0) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            stack[depth - 1]->attributes++;
            attributeStored = (tokenPosition < maxTokens);
            attribute = attributeStored ? &tokens[tokenPosition] : &attributeScratch;
            memset(attribute, 0, sizeof(*attribute));
            attribute->type = XML_TOKEN_ATTRIBUTE;
            size_t nameLength = yxml_symlen(&parser, parser.attr);
            if(attributeStored &&
               !XmlTextBuffer_appendName(text, parser.attr, nameLength,
                                         &attribute->name)) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            tokenPosition++;
            break;
        }

        case YXML_CONTENT:
            if(depth > 0 && stored[depth - 1] && !hasChildren[depth - 1]) {
                size_t length = strlen(parser.data);
                if(stack[depth - 1]->contentLength == 0)
                    stack[depth - 1]->content = text->size;
                if(!XmlTextBuffer_append(text, parser.data, length)) {
                    result.status = XML_TOKENIZE_INVALID;
                    result.tokensSize = tokenPosition;
                    return result;
                }
                stack[depth - 1]->contentLength += length;
            }
            break;

        case YXML_ATTRVAL:
            if(attributeStored) {
                size_t length = strlen(parser.data);
                if(attribute->contentLength == 0)
                    attribute->content = text->size;
                if(!XmlTextBuffer_append(text, parser.data, length)) {
                    result.status = XML_TOKENIZE_INVALID;
                    result.tokensSize = tokenPosition;
                    return result;
                }
                attribute->contentLength += length;
            }
            break;

        case YXML_ATTREND:
            if(attributeStored && attribute->contentLength > 0 &&
               !XmlTextBuffer_append(text, "", 1)) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            attribute = NULL;
            attributeStored = false;
            break;

        case YXML_ELEMEND:
            if(depth == 0) {
                result.status = XML_TOKENIZE_INVALID;
                result.tokensSize = tokenPosition;
                return result;
            }
            depth--;
            stack[depth]->end = pos + 1;
            stack[depth]->subtreeEnd = tokenPosition;
            if(stored[depth] && stack[depth]->contentLength > 0 &&
               !XmlTextBuffer_append(text, "", 1)) {
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
    if(yxml_eof(&parser) != YXML_OK || depth != 0) {
        result.status = XML_TOKENIZE_INVALID;
    } else if(tokenPosition > maxTokens) {
        result.status = XML_TOKENIZE_OVERFLOW;
    }
    return result;
}

typedef struct {
    Nodeset *nodeset;
} ParserContext;

static const char *
localName(const char *qualifiedName) {
    const char *colon = strrchr(qualifiedName, ':');
    return colon ? colon + 1 : qualifiedName;
}

static bool
isNamespaceAttribute(const char *name) {
    return !strcmp(name, "xmlns") || !strncmp(name, "xmlns:", 6);
}

typedef enum {
    XML_SCOPE_DOCUMENT,
    XML_SCOPE_NODE,
    XML_SCOPE_NAMESPACE_URIS,
    XML_SCOPE_REFERENCES,
    XML_SCOPE_DEFINITION
} XmlScope;

#define XML_ATTRIBUTE_STACK_SIZE 16
typedef struct {
    const XmlToken *tokens;
    size_t tokensSize;
    size_t position;
    const XmlTextBuffer *text;
    const char *xml;
    size_t xmlLength;
    const char **attributes;
    size_t attributesCapacity;
} XmlCursor;

static bool
XmlToken_nodeClass(const char *name, NL_NodeClass *nodeClass) {
    if(!strcmp(name, VARIABLE))
        *nodeClass = NODECLASS_VARIABLE;
    else if(!strcmp(name, OBJECT))
        *nodeClass = NODECLASS_OBJECT;
    else if(!strcmp(name, OBJECTTYPE))
        *nodeClass = NODECLASS_OBJECTTYPE;
    else if(!strcmp(name, DATATYPE))
        *nodeClass = NODECLASS_DATATYPE;
    else if(!strcmp(name, METHOD))
        *nodeClass = NODECLASS_METHOD;
    else if(!strcmp(name, REFERENCETYPE))
        *nodeClass = NODECLASS_REFERENCETYPE;
    else if(!strcmp(name, VARIABLETYPE))
        *nodeClass = NODECLASS_VARIABLETYPE;
    else if(!strcmp(name, VIEW))
        *nodeClass = NODECLASS_VIEW;
    else
        return false;
    return true;
}

static bool
XmlToken_attributes(XmlCursor *cursor, const XmlToken *element,
                    size_t attributePosition, const char ***attributes,
                    size_t *attributesSize) {
    *attributes = NULL;
    *attributesSize = 0;

    for(size_t i = 0; i < element->attributes; i++) {
        const XmlToken *attribute = &cursor->tokens[attributePosition + i];
        const char *name = cursor->text->data + attribute->name;
        if(!isNamespaceAttribute(name))
            (*attributesSize)++;
    }

    if(*attributesSize > (size_t)INT_MAX ||
       *attributesSize > cursor->attributesCapacity)
        return false;
    if(*attributesSize == 0)
        return true;

    size_t out = 0;
    for(size_t i = 0; i < element->attributes; i++) {
        const XmlToken *attribute = &cursor->tokens[attributePosition + i];
        const char *name = cursor->text->data + attribute->name;
        if(isNamespaceAttribute(name))
            continue;
        cursor->attributes[out * 5] = localName(name);
        cursor->attributes[out * 5 + 1] = NULL;
        cursor->attributes[out * 5 + 2] = NULL;
        cursor->attributes[out * 5 + 3] =
            cursor->text->data + attribute->content;
        cursor->attributes[out * 5 + 4] =
            cursor->attributes[out * 5 + 3] + attribute->contentLength;
        out++;
    }

    *attributes = cursor->attributes;
    return true;
}

static bool
XmlToken_copyContent(ParserContext *context, const XmlCursor *cursor,
                     const XmlToken *element, char **content) {
    *content = NULL;
    if(element->contentLength == 0)
        return true;
    if(element->contentLength == SIZE_MAX)
        return false;

    char *result = CharArenaAllocator_malloc(context->nodeset->charArena,
                                              element->contentLength + 1);
    if(!result)
        return false;
    memcpy(result, cursor->text->data + element->content,
           element->contentLength);
    result[element->contentLength] = 0;
    *content = result;
    return true;
}

static bool
XmlToken_copyLeafContent(ParserContext *context, XmlCursor *cursor,
                         const XmlToken *element, char **content) {
    cursor->position = element->subtreeEnd;
    return XmlToken_copyContent(context, cursor, element, content);
}

static bool
ProcessElement(ParserContext *context, XmlCursor *cursor, XmlScope scope,
               NL_Node *node);

static bool
ProcessChildren(ParserContext *context, XmlCursor *cursor,
                const XmlToken *element, XmlScope scope, NL_Node *node) {
    if(element->subtreeEnd < cursor->position ||
       element->subtreeEnd > cursor->tokensSize)
        return false;
    while(cursor->position < element->subtreeEnd) {
        if(!ProcessElement(context, cursor, scope, node))
            return false;
    }
    return cursor->position == element->subtreeEnd;
}

static bool
ProcessElement(ParserContext *context, XmlCursor *cursor, XmlScope scope,
               NL_Node *node) {
    if(cursor->position >= cursor->tokensSize)
        return false;

    const XmlToken *element = &cursor->tokens[cursor->position++];
    if(element->type != XML_TOKEN_ELEMENT ||
       element->subtreeEnd < cursor->position ||
       element->subtreeEnd > cursor->tokensSize)
        return false;

    size_t attributePosition = cursor->position;
    if(element->attributes > element->subtreeEnd - cursor->position)
        return false;
    for(size_t i = 0; i < element->attributes; i++) {
        if(cursor->tokens[attributePosition + i].type != XML_TOKEN_ATTRIBUTE)
            return false;
    }
    cursor->position += element->attributes;

    const char *name = localName(cursor->text->data + element->name);
    const char **attributes = NULL;
    size_t attributesSize = 0;

    if(scope == XML_SCOPE_DOCUMENT) {
        NL_NodeClass nodeClass;
        if(XmlToken_nodeClass(name, &nodeClass)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            NL_Node *newNode = Nodeset_newNode(context->nodeset, nodeClass,
                                               attributesSize, attributes);
            return ProcessChildren(context, cursor, element, XML_SCOPE_NODE,
                                   newNode);
        } else if(!strcmp(name, NAMESPACEURIS)) {
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_NAMESPACE_URIS, NULL);
        } else if(!strcmp(name, ALIAS)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Alias *alias = Nodeset_newAlias(context->nodeset, attributesSize,
                                            attributes);
            char *content = NULL;
            if(!alias || !XmlToken_copyLeafContent(context, cursor, element,
                                                   &content))
                return false;
            Nodeset_newAliasFinish(context->nodeset, alias, content);
            return true;
        } else if(!strcmp(name, "UANodeSet") ||
                  !strcmp(name, "Aliases")) {
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_DOCUMENT, NULL);
        }
    } else if(scope == XML_SCOPE_NODE) {
        if(!strcmp(name, DISPLAYNAME)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Nodeset_setDisplayName(context->nodeset, node, attributesSize,
                                   attributes);
            char *content = NULL;
            if(!XmlToken_copyLeafContent(context, cursor, element, &content))
                return false;
            Nodeset_DisplayNameFinish(context->nodeset, node, content);
            return true;
        } else if(!strcmp(name, REFERENCES)) {
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_REFERENCES, node);
        } else if(!strcmp(name, DESCRIPTION)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Nodeset_setDescription(context->nodeset, node, attributesSize,
                                   attributes);
            char *content = NULL;
            if(!XmlToken_copyLeafContent(context, cursor, element, &content))
                return false;
            Nodeset_DescriptionFinish(context->nodeset, node, content);
            return true;
        } else if(!strcmp(name, VALUE)) {
            cursor->position = element->subtreeEnd;
            if(node->nodeClass != NODECLASS_VARIABLE)
                return true;
            if(element->end < element->start ||
               element->end > cursor->xmlLength)
                return false;
            UA_String xmlValue = {
                element->end - element->start,
                (UA_Byte*)(uintptr_t)(cursor->xml + element->start)
            };
            return UA_String_copy(&xmlValue, &((NL_VariableNode*)node)->value) ==
                UA_STATUSCODE_GOOD;
        } else if(!strcmp(name, "Definition") &&
                  node->nodeClass == NODECLASS_DATATYPE) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Nodeset_addDataTypeDefinition(context->nodeset, node,
                                          attributesSize, attributes);
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_DEFINITION, node);
        } else if(!strcmp(name, INVERSENAME)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Nodeset_setInverseName(context->nodeset, node, attributesSize,
                                   attributes);
            char *content = NULL;
            if(!XmlToken_copyLeafContent(context, cursor, element, &content))
                return false;
            Nodeset_InverseNameFinish(context->nodeset, node, content);
            return true;
        }
    } else if(scope == XML_SCOPE_NAMESPACE_URIS) {
        if(!strcmp(name, NAMESPACEURI)) {
            char *content = NULL;
            if(!XmlToken_copyLeafContent(context, cursor, element, &content))
                return false;
            Nodeset_newNamespaceFinish(context->nodeset, content);
            return true;
        }
    } else if(scope == XML_SCOPE_REFERENCES) {
        if(!strcmp(name, REFERENCE)) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            NL_Reference *reference = Nodeset_newReference(
                context->nodeset, node, attributesSize, attributes);
            char *content = NULL;
            if(!reference ||
               !XmlToken_copyLeafContent(context, cursor, element, &content))
                return false;
            Nodeset_newReference_finish(context->nodeset, reference, content);
            return true;
        }
    } else if(scope == XML_SCOPE_DEFINITION) {
        if(!strcmp(name, "Field")) {
            if(!XmlToken_attributes(cursor, element, attributePosition,
                                    &attributes, &attributesSize))
                return false;
            Nodeset_addDataTypeField(context->nodeset, node, attributesSize,
                                     attributes);
            cursor->position = element->subtreeEnd;
            return true;
        }
    }

    /* Unknown elements are irrelevant together with their entire subtree. */
    cursor->position = element->subtreeEnd;
    return true;
}

static int
Parser_run(ParserContext *context, FILE *file) {
    /* Read entire file into memory */
    if(fseek(file, 0, SEEK_END) != 0)
        return 1;
    long fsize = ftell(file);
    if(fsize < 0 || fseek(file, 0, SEEK_SET) != 0)
        return 1;
    if((uintmax_t)fsize >= SIZE_MAX)
        return 1;
    size_t fileSize = (size_t)fsize;

    char *buf = (char*)malloc(fileSize + 1);
    if(!buf)
        return 1;

    size_t elems = fread(buf, 1, fileSize, file);
    if(ferror(file)) {
        free(buf);
        return 1;
    }
    buf[elems] = 0; /* Ensure null terminated */

    XmlToken tokenBuffer[64];
    XmlToken *tokens = tokenBuffer;
    size_t tokensCapacity = 64;
    /* Decoded names and leaf values cannot exceed their source XML size. */
    XmlTextBuffer text = {(char*)malloc(elems + 1), 0, elems + 1};
    if(!text.data) {
        free(buf);
        return 1;
    }

    XmlTokenizeResult result =
        XmlTokenize(buf, elems, tokens, tokensCapacity, &text);
    if(result.status == XML_TOKENIZE_OVERFLOW) {
        tokensCapacity = result.tokensSize;
        tokens = NULL;
        if(tokensCapacity <= SIZE_MAX / sizeof(XmlToken))
            tokens = (XmlToken*)malloc(tokensCapacity * sizeof(XmlToken));
        if(tokens)
            result = XmlTokenize(buf, elems, tokens, tokensCapacity, &text);
    }

    const char *attributeStack[XML_ATTRIBUTE_STACK_SIZE * 5];
    const char **attributeBuffer = attributeStack;
    size_t attributesCapacity = XML_ATTRIBUTE_STACK_SIZE;
    int ret = 1;
    if(tokens && result.status == XML_TOKENIZE_OK && result.tokensSize > 0) {
        for(size_t i = 0; i < result.tokensSize; i++) {
            if(tokens[i].type == XML_TOKEN_ELEMENT &&
               tokens[i].attributes > attributesCapacity)
                attributesCapacity = tokens[i].attributes;
        }
        if(attributesCapacity > XML_ATTRIBUTE_STACK_SIZE) {
            if(attributesCapacity <= SIZE_MAX / (5 * sizeof(char*)))
                attributeBuffer = (const char**)malloc(
                    attributesCapacity * 5 * sizeof(char*));
            else
                attributeBuffer = NULL;
        }

        XmlCursor cursor = {tokens, result.tokensSize, 0, &text, buf, elems,
                            attributeBuffer, attributesCapacity};
        if(attributeBuffer &&
           ProcessElement(context, &cursor, XML_SCOPE_DOCUMENT, NULL) &&
           cursor.position == cursor.tokensSize)
            ret = 0;
    }

    if(attributeBuffer != attributeStack)
        free(attributeBuffer);
    if(tokens != tokenBuffer)
        free(tokens);
    free(text.data);
    free(buf);
    return ret;
}

bool
NodesetLoader_importFile(NodesetLoader *loader,
                         const NL_FileContext *fileHandler) {
    if(!fileHandler) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: no filehandler - abort");
        return false;
    }

    if(!fileHandler->addNamespace) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: fileHandler->addNamespace missing");
        return false;
    }

    if(!loader->nodeset) {
        loader->nodeset = Nodeset_new(fileHandler->addNamespace, loader->logger);
    }

    ParserContext ctx;
    bool retStatus = true;
    FILE *f = fopen(fileHandler->file, "r");
    memset(&ctx, 0, sizeof(ctx));

    if(!f) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: file open error");
        retStatus = false;
        goto cleanup;
    }

    ctx.nodeset = loader->nodeset;
    ctx.nodeset->fc = (NL_FileContext*)(uintptr_t)fileHandler;

    if(Parser_run(&ctx, f)) {
        UA_LOG_ERROR(loader->logger, UA_LOGCATEGORY_SERVER,
                     "NodesetLoader: xml parsing error");
        retStatus = false;
    }

cleanup:
    if(f)
        fclose(f);
    return retStatus;
}

bool
NodesetLoader_sort(NodesetLoader *loader) {
    return Nodeset_sort(loader->nodeset);
}

NodesetLoader *
NodesetLoader_new(UA_Logger *logger) {
    if(!logger)
        return NULL;

    NodesetLoader *loader = (NodesetLoader *)calloc(1, sizeof(NodesetLoader));
    if(!loader)
        return NULL;
    loader->logger = logger;
    return loader;
}

void
NodesetLoader_delete(NodesetLoader *loader) {
    Nodeset_cleanup(loader->nodeset);
    free(loader);
}

bool
NodesetLoader_forEachNode(NodesetLoader *loader, void *context,
                          NodesetLoader_forEachNode_Func fn) {
    return Nodeset_forEachNode(loader->nodeset, context, fn);
}
