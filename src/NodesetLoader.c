/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "Nodeset.h"
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

struct NodesetLoader {
    Nodeset *nodeset;
    UA_Logger *logger;
};

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

typedef enum {
    XML_SCOPE_DOCUMENT,
    XML_SCOPE_NODE,
    XML_SCOPE_NAMESPACE_URIS,
    XML_SCOPE_REFERENCES,
    XML_SCOPE_DEFINITION
} XmlScope;

typedef struct {
    const XmlToken *tokens;
    size_t tokensSize;
    size_t position;
    const XmlTextBuffer *text;
    const char *xml;
    size_t xmlLength;
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

static char *
XmlToken_leafContent(XmlCursor *cursor, const XmlToken *element) {
    cursor->position = element->subtreeEnd;
    if(element->contentLength == 0)
        return NULL;
    return cursor->text->data + element->content;
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
    XmlAttributes attributes = {
        &cursor->tokens[attributePosition], element->attributes,
        cursor->text->data};

    if(scope == XML_SCOPE_DOCUMENT) {
        NL_NodeClass nodeClass;
        if(XmlToken_nodeClass(name, &nodeClass)) {
            NL_Node *newNode = Nodeset_newNode(context->nodeset, nodeClass,
                                               &attributes);
            return ProcessChildren(context, cursor, element, XML_SCOPE_NODE,
                                   newNode);
        } else if(!strcmp(name, NAMESPACEURIS)) {
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_NAMESPACE_URIS, NULL);
        } else if(!strcmp(name, ALIAS)) {
            Alias *alias = Nodeset_newAlias(context->nodeset, &attributes);
            char *content = XmlToken_leafContent(cursor, element);
            if(!alias)
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
            Nodeset_setDisplayName(node, &attributes);
            char *content = XmlToken_leafContent(cursor, element);
            Nodeset_DisplayNameFinish(node, content);
            return true;
        } else if(!strcmp(name, REFERENCES)) {
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_REFERENCES, node);
        } else if(!strcmp(name, DESCRIPTION)) {
            Nodeset_setDescription(node, &attributes);
            char *content = XmlToken_leafContent(cursor, element);
            Nodeset_DescriptionFinish(node, content);
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
            Nodeset_addDataTypeDefinition(node, &attributes);
            return ProcessChildren(context, cursor, element,
                                   XML_SCOPE_DEFINITION, node);
        } else if(!strcmp(name, INVERSENAME)) {
            Nodeset_setInverseName(node, &attributes);
            char *content = XmlToken_leafContent(cursor, element);
            Nodeset_InverseNameFinish(node, content);
            return true;
        }
    } else if(scope == XML_SCOPE_NAMESPACE_URIS) {
        if(!strcmp(name, NAMESPACEURI)) {
            char *content = XmlToken_leafContent(cursor, element);
            Nodeset_newNamespaceFinish(context->nodeset, content);
            return true;
        }
    } else if(scope == XML_SCOPE_REFERENCES) {
        if(!strcmp(name, REFERENCE)) {
            NL_Reference *reference = Nodeset_newReference(
                context->nodeset, node, &attributes);
            char *content = XmlToken_leafContent(cursor, element);
            if(!reference)
                return false;
            Nodeset_newReference_finish(context->nodeset, reference, content);
            return true;
        }
    } else if(scope == XML_SCOPE_DEFINITION) {
        if(!strcmp(name, "Field")) {
            Nodeset_addDataTypeField(context->nodeset, node, &attributes);
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

    bool textOwned = false;
    int ret = 1;
    if(tokens && result.status == XML_TOKENIZE_OK && result.tokensSize > 0) {
        XmlCursor cursor = {tokens, result.tokensSize, 0, &text, buf, elems};
        if(Nodeset_ownTextBuffer(context->nodeset, text.data)) {
            textOwned = true;
        }
        if(textOwned &&
           ProcessElement(context, &cursor, XML_SCOPE_DOCUMENT, NULL) &&
           cursor.position == cursor.tokensSize)
            ret = 0;
    }

    if(tokens != tokenBuffer)
        free(tokens);
    if(!textOwned)
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
        loader->nodeset = Nodeset_new(loader->logger);
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
    ctx.nodeset->fc = fileHandler;

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
    NodeContainer *nodes = &loader->nodeset->sortedNodes;
    for(size_t i = 0; i < nodes->size; i++) {
        if(!fn(context, nodes->nodes[i]))
            return false;
    }
    return true;
}
