#ifndef CUSTOMDATATYPE_H
#define CUSTOMDATATYPE_H
const struct UA_DataType *findCustomDataType(const UA_NodeId *typeId,
                                       const UA_DataTypeArray *types);

#endif