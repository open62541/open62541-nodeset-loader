/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef CONVERSION_H
#define CONVERSION_H
#include <open62541/types.h>

#include "NodesetLoader/NodesetLoader.h"

static inline UA_Boolean isValTrue(const char *s)
{
    if (!s)
    {
        return UA_FALSE;
    }
    if (strcmp(s, "true"))
    {
        return UA_FALSE;
    }
    return UA_TRUE;
}

#endif
