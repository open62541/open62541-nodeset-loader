/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef INTERNALLOGGER_H
#define INTERNALLOGGER_H

#include "NodesetLoader/Logger.h"

NodesetLoader_Logger *InternalLogger_new(void);
void InternalLogger_delete(NodesetLoader_Logger *logger);

#endif
