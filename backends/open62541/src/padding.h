/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef PADDING_H
#define PADDING_H

static inline UA_Byte getPadding(int alignment, int offset)
{
    return (UA_Byte)((alignment - (offset % alignment)) % alignment);
}

#endif
