/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef PARSER_H
#define PARSER_H
#include <stdio.h>

struct Parser;
typedef struct Parser Parser;

typedef void (*Parser_callbackStart)(void *ctx, const char *localname,
                                     const char *prefix, const char *URI,
                                     int nb_namespaces, const char **namespaces,
                                     int nb_attributes, int nb_defaulted,
                                     const char **attributes);

typedef void (*Parser_callbackEnd)(void *ctx, const char *localname,
                                   const char *prefix, const char *URI);

typedef void (*Parser_callbackChar)(void *ctx, const char *ch, int len);

Parser *Parser_new(void *context);
int Parser_run(Parser *parser, FILE *file, Parser_callbackStart start,
               Parser_callbackEnd end, Parser_callbackChar onChars);
void Parser_delete(Parser *parser);
#endif