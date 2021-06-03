#ifndef _VALUE_UTILS_H
#define _VALUE_UTILS_H

#include <fstream>
#include <open62541/types.h>

void PrintArrayDimensions(const size_t arrayDimSize,
                          const UA_UInt32 *const pArrayDimensions,
                          const UA_UInt32 Indentation, std::ostream &out);

UA_Boolean PrintValueAttribute(const UA_Variant &Value, std::ostream &out);

#endif // _VALUE_UTILS_H