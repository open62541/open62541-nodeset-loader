#ifndef DATATYPEIMPORTER_H
#define DATATYPEIMPORTER_H
#include <NodesetLoader/NodesetLoader.h>

struct DataTypeImporter;
typedef struct DataTypeImporter DataTypeImporter;

struct UA_Server;
struct UA_DataType;

DataTypeImporter *DataTypeImporter_new(struct UA_Server *server);
void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const TDataTypeNode *node, const struct UA_DataType* parent);
// has to be called after all dependent types where added
void DataTypeImporter_initMembers(DataTypeImporter *importer);
void DataTypeImporter_delete(DataTypeImporter *importer);

#endif
