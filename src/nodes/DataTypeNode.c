/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "DataTypeNode.h"
#include <assert.h>
#include <stdlib.h>

static DataTypeDefinitionField *getNewField(DataTypeDefinition *definition)
{
    definition->fieldCnt++;
    definition->fields = (DataTypeDefinitionField *)realloc(
        definition->fields,
        definition->fieldCnt * sizeof(DataTypeDefinitionField));
    return &definition->fields[definition->fieldCnt - 1];
}

DataTypeDefinitionField *DataTypeNode_addDefinitionField(TDataTypeNode *node)
{
    if (!node->definition)
    {
        node->definition =
            (DataTypeDefinition *)calloc(1, sizeof(DataTypeDefinition));
        assert(node->definition);
    }

    return getNewField(node->definition);
}

void DataTypeNode_clear(TDataTypeNode *node)
{
    if (node->definition)
    {
        free(node->definition->fields);
    }
    free(node->definition);
}
