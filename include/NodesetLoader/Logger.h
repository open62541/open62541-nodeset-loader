/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_LOGGER_H
#define NODESETLOADER_LOGGER_H

struct NodesetLoader_Logger;
typedef struct NodesetLoader_Logger NodesetLoader_Logger;

enum NodesetLoader_LogLevel
{
    NODESETLOADER_LOGLEVEL_DEBUG,
    NODESETLOADER_LOGLEVEL_WARNING,
    NODESETLOADER_LOGLEVEL_ERROR
};

typedef void (*NodesetLoader_Logger_log)(void *context,
                                         enum NodesetLoader_LogLevel level,
                                         const char *message, ...);
struct NodesetLoader_Logger
{
    void *context;
    NodesetLoader_Logger_log log;
};

#endif
