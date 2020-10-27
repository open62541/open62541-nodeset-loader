/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef NODESETLOADER_ARCH_H
#define NODESETLOADER_ARCH_H

/* this is taken from open62541.com */
#if defined(_WIN32) && defined(UA_DYNAMIC_LINKING)
#ifdef UA_DYNAMIC_LINKING_EXPORT /* export dll */
#ifdef __GNUC__
#define LOADER_EXPORT __attribute__((dllexport))
#else
#define UA_EXPORT __declspec(dllexport)
#endif
#else /* import dll */
#ifdef __GNUC__
#define LOADER_EXPORT __attribute__((dllimport))
#else
#define LOADER_EXPORT __declspec(dllimport)
#endif
#endif
#else /* non win32 */
#if __GNUC__ || __clang__
#define LOADER_EXPORT __attribute__((visibility("default")))
#endif
#endif

#ifndef LOADER_EXPORT
#define LOADER_EXPORT /* fallback to default */
#endif

#endif
