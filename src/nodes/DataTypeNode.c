/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "DataTypeNode.h"
#include <stdlib.h>

static NL_DataTypeDefinitionField *getNewField(NL_DataTypeDefinition *definition)
{
    definition->fieldCnt++;
    definition->fields = (NL_DataTypeDefinitionField *)realloc(
        definition->fields,
        definition->fieldCnt * sizeof(NL_DataTypeDefinitionField));
    if(!definition->fields)
    {
        return NULL;
    }
    return &definition->fields[definition->fieldCnt - 1];
}

NL_DataTypeDefinition* DataTypeDefinition_new(NL_DataTypeNode* node)
{
    node->definition =
        (NL_DataTypeDefinition *)calloc(1, sizeof(NL_DataTypeDefinition));
    if (!node->definition)
    {
        return NULL;
    }
    return node->definition;
}

NL_DataTypeDefinitionField *DataTypeNode_addDefinitionField(NL_DataTypeDefinition *def)
{
    return getNewField(def);
}

void DataTypeNode_clear(NL_DataTypeNode *node)
{
    if (node->definition)
    {
        free(node->definition->fields);
    }
    free(node->definition);
}
