#ifndef CUSTOMDATATYPE_H
#define CUSTOMDATATYPE_H
const struct UA_DataType *findDataType(const UA_NodeId *typeId,
                                       const UA_DataTypeArray *types);

#endif