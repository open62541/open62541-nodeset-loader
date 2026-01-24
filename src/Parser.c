/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 *    Copyright 2025 (c) SICK AG (Author: Joerg Fischer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "Parser.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/xmlreader.h>

int
Parser_run(void *context, FILE *file,
           Parser_callbackStart start,
           Parser_callbackEnd end,
           Parser_callbackChar onChars) {
    assert(start);
    assert(end);
    assert(onChars);

    /* Read entire file into memory */
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buf = (char*)malloc((size_t)(fsize + 1));
    if(!buf)
        return 1;

    size_t elems = fread(buf, 1, (size_t)fsize, file);
    buf[elems] = 0; /* Ensure null terminated */

    xmlInitParser();

    xmlTextReaderPtr reader =
        xmlReaderForMemory(buf, (int)elems,
                           NULL, /* Filename not needed */
                           "UTF-8", XML_PARSE_HUGE);

    if(reader == NULL) {
        free(buf);
        return 1;
    }

    int ret;
    while((ret = xmlTextReaderRead(reader)) == 1) {
        int nodeType = xmlTextReaderNodeType(reader);

        if(nodeType == XML_READER_TYPE_ELEMENT) {
            const char *local = (const char*)xmlTextReaderConstLocalName(reader);
            const char *prefix = (const char*)xmlTextReaderConstPrefix(reader);
            const char *uri = (const char*)xmlTextReaderConstNamespaceUri(reader);

            /* namespace declarations:
             * xmlTextReader does not directly expose declared ns on this node,
             * but for most OPC UA nodesets (and SAX2 consumers),
             * the namespaces array was rarely used beyond URI+prefix.
             * We pass 0 here. If you need full ns scope, we can augment via
             * xmlGetNsList(document,node). */
            size_t nb_namespaces = 0;
            const char **namespaces = NULL;

            /* attributes */
            size_t nb_attributes = 0;
            size_t nb_defaulted = 0;

            if(xmlTextReaderHasAttributes(reader)) {
                xmlTextReaderMoveToFirstAttribute(reader);
                do {
                    nb_attributes++;
                } while(xmlTextReaderMoveToNextAttribute(reader) == 1);

                /* Now build classical SAX2 attributes array:
                 * attributes is:
                 *  [localname, prefix, URI, value, ...] 4-tuple repeated
                 */
                const char **attrs = (const char **)
                    malloc(sizeof(char*) * nb_attributes * 5);
                int idx = 0;

                xmlTextReaderMoveToFirstAttribute(reader);
                do {
                    const char *_local  = (const char*)xmlTextReaderConstLocalName(reader);
                    const char *_prefix = (const char*)xmlTextReaderConstPrefix(reader);
                    const char *_uri    = (const char*)xmlTextReaderConstNamespaceUri(reader);
                    const char *_value  = (const char*)xmlTextReaderConstValue(reader);
                    attrs[idx++] = _local;
                    attrs[idx++] = _prefix;
                    attrs[idx++] = _uri;
                    attrs[idx++] = _value;
                    attrs[idx++] = _value ? (_value + strlen(_value)) : NULL;
                } while(xmlTextReaderMoveToNextAttribute(reader) == 1);

                start(context, local, prefix, uri, nb_namespaces, namespaces,
                      nb_attributes, nb_defaulted, attrs);

                free(attrs);
                xmlTextReaderMoveToElement(reader);
            } else {
                start(context, local, prefix, uri, nb_namespaces, namespaces,
                      nb_attributes, nb_defaulted, NULL);
            }

            if(xmlTextReaderIsEmptyElement(reader))
                end(context, local, prefix, uri);
        } else if(nodeType == XML_READER_TYPE_END_ELEMENT) {
            const char *local = (const char*)xmlTextReaderConstLocalName(reader);
            const char *prefix = (const char*)xmlTextReaderConstPrefix(reader);
            const char *uri = (const char*)xmlTextReaderConstNamespaceUri(reader);
            end(context, local, prefix, uri);
        } else if(nodeType == XML_READER_TYPE_TEXT ||
                nodeType == XML_READER_TYPE_SIGNIFICANT_WHITESPACE ||
                nodeType == XML_READER_TYPE_WHITESPACE) {
            const char *txt = (const char*)xmlTextReaderConstValue(reader);
            if(txt && *txt)
                onChars(context, txt, strlen(txt));
        }
    }

    xmlFreeTextReader(reader);
    xmlCleanupParser();
    free(buf);

    if(ret < 0)
        return 1;
    return 0;
}
