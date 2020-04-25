#include "DataTypeNode.h"
#include <stdlib.h>
#include <assert.h>

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
    if(node->definition)
    {
        free(node->definition->fields);
    }    
    free(node->definition);
}