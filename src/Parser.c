/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 *    Copyright 2025 (c) SICK AG (Author: Joerg Fischer)
 */

#include "Parser.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Read the entire file into a buffer. So we can access entire sub-xml trees
 * via the "consumed" index. */
int
Parser_run(TParserCtx *parser, FILE *file, Parser_callbackStart start,
           Parser_callbackEnd end, Parser_callbackChar onChars) {
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    parser->buf = (char*)malloc((size_t)(fsize + 1));
    size_t elems = fread(parser->buf, 1, (size_t)fsize+1, file);
    if (elems == 0)
        return 1;

    xmlSAXHandler hdl;
    memset(&hdl, 0, sizeof(xmlSAXHandler));
    hdl.initialized = XML_SAX2_MAGIC;

    // nodesets are encoded with UTF-8
    // this code does no transformation on the encoded text or interprets it
    // so it should be safe to cast xmlChar* to char*
    hdl.startElementNs = (startElementNsSAX2Func)start;
    hdl.endElementNs = (endElementNsSAX2Func)end;
    hdl.characters = (charactersSAXFunc)onChars;

    xmlInitParser(); // Fix memory leak: https://gitlab.gnome.org/GNOME/libxml2/-/issues/9

    parser->ctxt = xmlCreatePushParserCtxt(&hdl, parser, NULL, 0, NULL);
    xmlCtxtUseOptions(parser->ctxt, XML_PARSE_HUGE);
    int ret = xmlParseChunk(parser->ctxt, parser->buf, (int)elems, 1);
    if(ret != 0) {
        const xmlError *err = xmlGetLastError();
        xmlParserError(parser->ctxt, "xml parse error %i %s", ret, err->message);
        free(parser->buf);
        parser->buf = NULL;
        return 1;
    }

    xmlFreeParserCtxt(parser->ctxt);
    xmlCleanupParser();
    free(parser->buf);
    parser->buf = NULL;
    return 0;
}
