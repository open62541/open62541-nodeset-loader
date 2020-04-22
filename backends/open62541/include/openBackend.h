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

struct UA_Server;
bool NodesetLoader_loadFile(struct UA_Server *, const char *path,
                            void *extensionHandling);

#endif