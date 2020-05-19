/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_ARCH_H
#define NODESETLOADER_ARCH_H

#if __GNUC__ || __clang__
#define LOADER_EXPORT __attribute__((visibility("default")))
#endif
#ifndef LOADER_EXPORT
#define LOADER_EXPORT /* fallback to default */
#endif

#endif
