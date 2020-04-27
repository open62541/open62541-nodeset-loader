#ifndef DATATYPENODE_H
#define DATATYPENODE_H
#include <NodesetLoader/NodesetLoader.h>

void DataTypeNode_clear(TDataTypeNode *node);
DataTypeDefinitionField *DataTypeNode_addDefinitionField(TDataTypeNode *node);
#endif
