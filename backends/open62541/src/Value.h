/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef VALUE_H
#define VALUE_H

#include <open62541/types.h>

#include "NodesetLoader/NodesetLoader.h"

struct ServerContext;

struct RawData;
struct RawData
{
    void *mem;
    size_t offset;
    struct RawData* next;
    //e.g. for ByteString
    void* additionalMem;
};
typedef struct RawData RawData;
RawData *RawData_new(RawData *old);
void RawData_delete(RawData *data);

void Value_getData(RawData *outData, const NL_Value *value, const UA_DataType* type, const UA_DataType* customTypes, const struct ServerContext *serverContext);

#endif
