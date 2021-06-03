#include "value_utils.h"
#include "common_defs.h"
#include "operator_ov.h"

#include <open62541/client.h>
#include <string>
#include <string_view>

using namespace std;

void PrintArrayDimensions(const size_t arrayDimSize,
                          const UA_UInt32 *const pArrayDimensions,
                          const UA_UInt32 Indentation, ostream &out)
{
    string strIndent = string(Indentation, '\t');
    out << strIndent << "ArrayDimensions = [";
    if (pArrayDimensions != 0)
    {
        for (size_t i = 0; i < arrayDimSize; i++)
        {
            out << strIndent << "\t" << pArrayDimensions[i] << ",";
        }
    }
    out << "]" << endl;
}

// Note: DataTypeId is necessary for ExtensionObject, otherwise we can't get
// the type Id
UA_Boolean PrintValueAttribute(const UA_Variant &Value, std::ostream &out)
{
    out << "printing value omitted" << "\n";
    return UA_TRUE;
}
