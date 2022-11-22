/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#ifndef DATATYPEIMPORTER_H
#define DATATYPEIMPORTER_H

#include <open62541/server.h>

#include "NodesetLoader/NodesetLoader.h"

struct DataTypeImporter;
typedef struct DataTypeImporter DataTypeImporter;

DataTypeImporter *DataTypeImporter_new(struct UA_Server *server);
void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const NL_DataTypeNode *node, const UA_NodeId parentId);
// has to be called after all dependent types where added
void DataTypeImporter_initMembers(DataTypeImporter *importer);
void DataTypeImporter_delete(DataTypeImporter *importer);

#endif
