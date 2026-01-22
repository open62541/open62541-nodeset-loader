/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef __NODESETLOADER_BACKEND_OPEN62541_H__
#define __NODESETLOADER_BACKEND_OPEN62541_H__

#include <open62541/server.h>

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

UA_EXPORT bool
NodesetLoader_loadFile(struct UA_Server *, const char *path,
                       void *options);

#ifdef __cplusplus
}
#endif
#endif
