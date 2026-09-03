/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_NODESETLOADER_H
#define NODESETLOADER_NODESETLOADER_H

#include <open62541/server.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Individual nodes that cannot be added are logged and skipped. The return
 * status reports fatal loading failures. */
UA_EXPORT UA_StatusCode UA_Server_loadNodeset(UA_Server *server,
                                              const char *nodeset2XmlFilePath);

#ifdef __cplusplus
}
#endif
#endif
