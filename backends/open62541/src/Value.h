/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef VALUE_H
#define VALUE_H
#include <open62541/types.h>
#include <NodesetLoader/NodesetLoader.h>

struct RawData
{
    void *mem;
    size_t offset;
};
typedef struct RawData RawData;
void RawData_delete(RawData *data);

RawData *Value_getData(const Value *value, const UA_DataType* type, const UA_DataType* customTypes);

#endif
