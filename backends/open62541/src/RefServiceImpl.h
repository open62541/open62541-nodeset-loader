/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef REFSERVICEIMPL_H
#define REFSERVICEIMPL_H

#include <open62541/server.h>
#include "NodesetLoader/ReferenceService.h"

NL_ReferenceService *RefServiceImpl_new(struct UA_Server *server);
void RefServiceImpl_delete(NL_ReferenceService *service);
#endif
