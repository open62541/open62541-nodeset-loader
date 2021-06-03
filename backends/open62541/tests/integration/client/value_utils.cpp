#include "value_utils.h"
#include "common_defs.h"
#include "operator_ov.h"

#include <open62541/client.h>
#include <string>
#include <string_view>

using namespace std;

// // note: Input-OutputArguments: Variant contains TypeId of ExtensionObject
// // which does not provide information about the type ...
// // decoded .. typeId?

// UA_Boolean PrintVariant(UA_Client *pClient, const UA_NodeId &DataTypeId,
//                         UA_DataTypeMember *pDataTypeMembers,
//                         size_t &CurrentMember, size_t NestedStructureDepth,
//                         size_t ArrayLen, void *pData, size_t &DataOffset,
//                         const UA_UInt32 Indentation, ostream &out);

// // TODO: rework cout, out -> error messages
// // TODO: rework return values
// // TODO: refactor param names

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
    UA_String sVal = UA_STRING_NULL;
    UA_print(&Value, &UA_TYPES[UA_TYPES_VARIANT], &sVal);
    out << std::string_view{reinterpret_cast<char *>(sVal.data), sVal.length} << "\n";
    UA_String_clear(&sVal);

    return UA_TRUE;
}
