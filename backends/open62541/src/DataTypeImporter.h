#ifndef DATATYPEIMPORTER_H
#define DATATYPEIMPORTER_H
#include <nodesetLoader/NodesetLoader.h>

struct DataTypeImporter;
typedef struct DataTypeImporter DataTypeImporter;

struct UA_Server;

DataTypeImporter *DataTypeImporter_new(struct UA_Server *server);
void DataTypeImporter_addCustomDataType(DataTypeImporter *importer,
                                        const TDataTypeNode *node);
// has to be called after all types where
void DataTypeImporter_initTypes(DataTypeImporter *importer);
void DataTypeImporter_delete(DataTypeImporter *importer);

#endif
