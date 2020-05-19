/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "Parser.h"
#include <assert.h>
#include <libxml/SAX.h>
#include <stdlib.h>
#include <string.h>

struct Parser
{
    void *context;
};

Parser *Parser_new(void *context)
{
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    assert(parser);
    parser->context = context;
    return parser;
}

int Parser_run(Parser *parser, FILE *file, Parser_callbackStart start,
               Parser_callbackEnd end, Parser_callbackChar onChars)
{
    char chars[1024];
    int res = (int)fread(chars, 1, 4, file);
    if (res <= 0)
    {
        return 1;
    }

    xmlSAXHandler hdl;
    memset(&hdl, 0, sizeof(xmlSAXHandler));
    hdl.initialized = XML_SAX2_MAGIC;
    // nodesets are encoded with UTF-8
    // this code does no transformation on the encoded text or interprets it
    // so it should be safe to cast xmlChar* to char*
    hdl.startElementNs = (startElementNsSAX2Func)start;
    hdl.endElementNs = (endElementNsSAX2Func)end;
    hdl.characters = (charactersSAXFunc)onChars;
    xmlParserCtxtPtr ctxt =
        xmlCreatePushParserCtxt(&hdl, parser->context, chars, res, NULL);
    while ((res = (int)fread(chars, 1, sizeof(chars), file)) > 0)
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
void Parser_delete(Parser *parser) { free(parser); }
