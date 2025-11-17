/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef PARSER_H
#define PARSER_H

#include "NodesetLoader/NodesetLoader.h"
#include "Nodeset.h"
#include <libxml/SAX2.h>
#include <stdio.h>

typedef enum {
    PARSER_STATE_INIT,
    PARSER_STATE_NODE,
    PARSER_STATE_DISPLAYNAME,
    PARSER_STATE_REFERENCES,
    PARSER_STATE_REFERENCE,
    PARSER_STATE_DESCRIPTION,
    PARSER_STATE_INVERSENAME,
    PARSER_STATE_ALIAS,
    PARSER_STATE_NAMESPACEURIS,
    PARSER_STATE_URI,
    PARSER_STATE_VALUE,
    PARSER_STATE_EXTENSION,
    PARSER_STATE_EXTENSIONS,
    PARSER_STATE_DATATYPE_DEFINITION,
    PARSER_STATE_DATATYPE_DEFINITION_FIELD
} TParserState;

struct TParserCtx {
    void *userContext;
    TParserState state;
    size_t unknown_depth;
    size_t value_depth;
    NL_NodeClass nodeClass;
    NL_Node *node;
    struct Alias *alias;
    char *onCharacters;
    size_t onCharLength;
    long valueBegin;
    void *extensionData;
    NodesetLoader_ExtensionInterface *extIf;
    NL_Reference *ref;
    Nodeset *nodeset;
    xmlParserCtxtPtr ctxt;
    char *buf;
};

typedef void (*Parser_callbackStart)(void *ctx, const char *localname,
                                     const char *prefix, const char *URI,
                                     int nb_namespaces, const char **namespaces,
                                     int nb_attributes, int nb_defaulted,
                                     const char **attributes);

typedef void (*Parser_callbackEnd)(void *ctx, const char *localname,
                                   const char *prefix, const char *URI);

typedef void (*Parser_callbackChar)(void *ctx, const char *ch, int len);

int Parser_run(TParserCtx *parser, FILE *file, Parser_callbackStart start,
               Parser_callbackEnd end, Parser_callbackChar onChars);

#endif
