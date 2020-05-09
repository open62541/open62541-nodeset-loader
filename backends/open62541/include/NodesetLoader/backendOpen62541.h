/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef __BACKEND_H__
#define __BACKEND_H__
#include <stdbool.h>
#include <stdio.h>

#if __GNUC__ || __clang__
#define LOADER_EXPORT __attribute__((visibility("default")))
#endif
#ifndef LOADER_EXPORT
#define LOADER_EXPORT /* fallback to default */
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct UA_Server;

LOADER_EXPORT bool NodesetLoader_loadFile(struct UA_Server *, const char *path,
                            void *extensionHandling);

#ifdef __cplusplus
}
#endif
#endif
