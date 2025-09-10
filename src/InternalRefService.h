/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef INTERNALREFSERVICE_H
#define INTERNALREFSERVICE_H

#include "NodesetLoader/NodesetLoader.h"

NL_ReferenceService *InternalRefService_new(void);
void InternalRefService_delete(NL_ReferenceService *service);
#endif
