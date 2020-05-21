#include "value_utils.h"
#include "common_defs.h"
#include "operator_ov.h"

#include <open62541/client_highlevel.h>
#include <string>

using namespace std;

// note: Input-OutputArguments: Variant contains TypeId of ExtensionObject
// which does not provide information about the type ...
// decoded .. typeId?

UA_Boolean PrintVariant(UA_Client *pClient, const UA_NodeId &DataTypeId,
                        UA_DataTypeMember *pDataTypeMembers,
                        size_t &CurrentMember, size_t NestedStructureDepth,
                        size_t ArrayLen, void *pData, size_t &DataOffset,
                        const UA_UInt32 Indentation, ostream &out);

// TODO: rework cout, out -> error messages
// TODO: rework return values
// TODO: refactor param names

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

// at the end of the day everything should be a built-in type to print
UA_Boolean PrintBuiltInType(const UA_UInt32 TypeIndex, const void *const pData,
                            size_t &DataOffset, const UA_UInt32 Indentation,
                            ostream &out)
{
    UA_Boolean bRet = UA_TRUE;

    string strIndent = string(Indentation, '\t');
    out << strIndent;

    UA_Byte *pTmp = ((UA_Byte *)pData) + DataOffset;

    switch (TypeIndex)
    {
    case UA_TYPES_BOOLEAN:
        out << ((*(UA_Boolean *)pTmp) ? "true" : "false");
        break;
    case UA_TYPES_BYTE:
        out << (*(UA_Byte *)pTmp);
        break;
    case UA_TYPES_UINT16:
        out << (*(UA_UInt16 *)pTmp);
        break;
    case UA_TYPES_UINT32:
        out << (*(UA_UInt32 *)pTmp);
        break;
    case UA_TYPES_UINT64:
        out << (*(UA_UInt64 *)pTmp);
        break;
    case UA_TYPES_SBYTE:
        out << (*(UA_SByte *)pTmp);
        break;
    case UA_TYPES_INT16:
        out << (*(UA_Int16 *)pTmp);
        break;
    case UA_TYPES_INT32:
        out << (*(UA_Int32 *)pTmp);
        break;
    case UA_TYPES_INT64:
        out << (*(UA_Int64 *)pTmp);
        break;
    case UA_TYPES_FLOAT:
        out << (*(UA_Float *)pTmp);
        break;
    case UA_TYPES_DOUBLE:
        out << (*(UA_Double *)pTmp);
        break;
    case UA_TYPES_STRING:
        out << (*(UA_String *)pTmp);
        break;
    case UA_TYPES_DATETIME:
        /* Time values (BuildDate etc.) do not make sense to print and
            compare with other servers, at least most of the time ... */
        out << "some DateTime Value";
        break;
    case UA_TYPES_GUID:
        out << (*(UA_Guid *)pTmp);
        break;
    case UA_TYPES_BYTESTRING:
        out << (*(UA_ByteString *)pTmp);
        break;
    case UA_TYPES_XMLELEMENT:
        out << (*(UA_XmlElement *)pTmp);
        break;
    case UA_TYPES_NODEID:
        out << (*(UA_NodeId *)pTmp);
        break;
    case UA_TYPES_EXPANDEDNODEID:
        out << (*(UA_ExpandedNodeId *)pTmp);
        break;
    case UA_TYPES_STATUSCODE:
        out << (*(UA_StatusCode *)pTmp);
        break;
    case UA_TYPES_DIAGNOSTICINFO:
        out << (*(UA_DiagnosticInfo *)pTmp);
        break;
    case UA_TYPES_QUALIFIEDNAME:
        out << (*(UA_QualifiedName *)pTmp);
        break;
    case UA_TYPES_LOCALIZEDTEXT:
        out << (*(UA_LocalizedText *)pTmp);
        break;
    default:
        cout << "PrintBuiltInType() Error: TypeIndex = '" << TypeIndex
             << "' not handled";
        bRet = UA_FALSE;
        break;
    }
    DataOffset += UA_TYPES[TypeIndex].memSize;
    out << endl;
    return bRet;
}

/* Note: the client does not have custom types compiled ...
for the first structure hierarchy we get the DataTypeMember information
from the variant ...

if we have nested structures -> do we need to read the DataTypeDefinition
attribute? to be able to handle them? */

UA_Boolean PrintStructure(UA_Client *pClient, const UA_NodeId &DataTypeId,
                          UA_DataTypeMember *pDataTypeMembers,
                          size_t &CurrentMember, size_t NestedStructureDepth,
                          void *pData, size_t &DataOffset,
                          const UA_UInt32 Indentation, ostream &out)
{
    UA_Boolean bRet = UA_TRUE;

    string strIndent = string(Indentation, '\t');
    out << strIndent << "{" << endl;

    // Read DataTypeDefinition attribute
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead = UA_ReadValueId_new();
    UA_ReadValueId_init(rReq.nodesToRead);
    UA_NodeId_copy(&DataTypeId, &rReq.nodesToRead->nodeId);
    rReq.nodesToRead->attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;

    UA_ReadResponse rResp;
    UA_ReadResponse_init(&rResp);
    rResp = UA_Client_Service_read(pClient, rReq);
    if ((rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD) &&
        (rResp.resultsSize == 1) && (rResp.results[0].hasValue == UA_TRUE) &&
        (UA_Variant_hasScalarType(&rResp.results[0].value,
                                  &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])))
    {
        UA_StructureDefinition *pDefinition =
            (UA_StructureDefinition *)rResp.results[0].value.data;
        size_t ArrayLen = 0;

        for (size_t i = 0; i < pDefinition->fieldsSize; i++)
        {
            if ((NestedStructureDepth == 1) && (pDataTypeMembers != 0))
            {
                // use padding information
                DataOffset += pDataTypeMembers[CurrentMember].padding;

                ArrayLen = 0;
                if (pDataTypeMembers[CurrentMember].isArray)
                {
                    // example: Argument:

                    /*
                        0x78: ValueRank
                        0x7C: 4 byte padding

                        0x80: &((UA_Argument*) pData)->arrayDimensionsSize
                        0x88: &((UA_Argument*) pData)->arrayDimensions
                        0x90: &((UA_Argument*) pData)->description

                    */

                    /* code from ua_types_encoding_binary.c

                    Array. Buffer-exchange is done inside Array_encodeBinary if
                    required. if(m->isArray) { const size_t length = *((const
                    size_t*)ptr); ptr += sizeof(size_t); ret =
                    Array_encodeBinary(*(void *UA_RESTRICT const *)ptr, length,
                    mt, ctx); ptr += sizeof(void*); continue;
                    }

                    */

                    UA_Byte *pTmp = ((UA_Byte *)pData) + DataOffset;
                    ArrayLen = *((const size_t *)pTmp);
                    DataOffset += sizeof(
                        size_t); // isn't sizeof(size_t) compiler dependent??
                    // TODO: Magic offset +12 would work
                }

                if (pDataTypeMembers[CurrentMember].isOptional)
                {
                    // TODO
                    out << "Attention: optional member is not handled" << endl;
                    return UA_FALSE;
                }
            }

            // TODO: how to handle optional members??
            bRet = PrintVariant(pClient, pDefinition->fields[i].dataType,
                                pDataTypeMembers, CurrentMember,
                                NestedStructureDepth + 1, ArrayLen, pData,
                                DataOffset, Indentation, out);

            if ((NestedStructureDepth == 1) && (pDataTypeMembers != 0))
            {
                // offset correction for arrays
                if (pDataTypeMembers[CurrentMember].isArray)
                {
                    /* TODO: don't know why but open62541 encodes it this way
                        ... -> does not work */
                    // DataOffset += sizeof(void *);
                    DataOffset += 4; // TODO
                }

                CurrentMember++;
            }
        }
    }
    else
    {
        cout << "PrintStructure() Error: Reading DataTypeDefinition "
                "attribute failed"
             << endl;
        bRet = UA_FALSE;
    }

    UA_ReadRequest_clear(&rReq);
    UA_ReadResponse_clear(&rResp);

    out << strIndent << "}" << endl;

    return bRet;
}

bool PrintExtensionObject(UA_Client *pClient, void *pData, size_t &DataOffset,
                          size_t Indentation, ostream &out)
{
    bool bRet = UA_TRUE;

    const UA_DataType *pType = 0;
    UA_NodeId DataTypeId;
    UA_NodeId_init(&DataTypeId);
    void *pStructureData = 0;

    UA_ExtensionObject *pExObj =
        (UA_ExtensionObject *)(((UA_Byte *)pData) + DataOffset);

    switch (pExObj->encoding)
    {
    case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
        cout << "PrintExtensionObject() Error: type 'encoded nobody' is not "
                "supported"
             << endl;
        bRet = UA_FALSE;
        break;
    case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
    case UA_EXTENSIONOBJECT_ENCODED_XML:
        UA_NodeId_copy(&pExObj->content.encoded.typeId, &DataTypeId);
        pType = UA_findDataType(&pExObj->content.encoded.typeId);
        pStructureData = pExObj->content.encoded.body.data;
        break;
    case UA_EXTENSIONOBJECT_DECODED:
    case UA_EXTENSIONOBJECT_DECODED_NODELETE: {
        UA_NodeId_copy(&pExObj->content.decoded.type->typeId, &DataTypeId);
        pType = UA_findDataType(&pExObj->content.decoded.type->typeId);
        pStructureData = pExObj->content.decoded.data;
        break;
    }
    default:
        cout << "PrintExtensionObject() Error: Unknown extension object "
                "encoding"
             << endl;
        bRet = UA_FALSE;
        break;
    }

    if ((bRet == UA_TRUE) && (pType != 0) && (pStructureData != 0))
    {
        size_t tmpCurrentMember = 0;
        size_t tmpDataOffset = 0;
        PrintStructure(pClient, pType->typeId, pType->members, tmpCurrentMember,
                       1, pStructureData, tmpDataOffset, Indentation, out);

        DataOffset += UA_TYPES[UA_TYPES_EXTENSIONOBJECT].memSize;
    }
    else
    {
        out << "Info: print extension object without type member "
               "information - does not work"
            << endl;

        /* TODO: we somehow cannot print extension object without
         * DataTypeMember information ...
         */

        /*
                // TODO: this is a test
                size_t tmpCurrentMember = 0;
                size_t tmpDataOffset = 0;
                PrintStructure(pClient, pType->typeId, 0, tmpCurrentMember,
           1, pStructureData, tmpDataOffset, Indentation, out);

        */
        DataOffset += UA_TYPES[UA_TYPES_EXTENSIONOBJECT].memSize;

        // bRet = UA_FALSE;
    }

    UA_NodeId_clear(&DataTypeId);

    return bRet;
}

UA_Boolean PrintVariant(UA_Client *pClient, const UA_NodeId &DataTypeId,
                        UA_DataTypeMember *pDataTypeMembers,
                        size_t &CurrentMember, size_t NestedStructureDepth,
                        size_t ArrayLen, void *pData, size_t &DataOffset,
                        const UA_UInt32 Indentation, ostream &out)
{
    UA_Boolean bRet = UA_TRUE;

    if (ArrayLen == 0)
    {
        ArrayLen = 1;
    }

    for (size_t i = 0; i < ArrayLen; i++)
    {
        if (UA_NodeId_equal(&DataTypeId, &StructureId) ==
            UA_TRUE) // special handling for extensionobjects
        {
            bRet = PrintExtensionObject(pClient, pData, DataOffset,
                                        Indentation + 1, out);
        }
        else if (IsSubType(pClient, StructureId, DataTypeId) ==
                 UA_TRUE) // handle "regular" structures
        {
            bRet = PrintStructure(pClient, DataTypeId, pDataTypeMembers,
                                  CurrentMember, NestedStructureDepth + 1,
                                  pData, DataOffset, Indentation + 1, out);
        }
        else if (IsSubType(pClient, EnumerationId, DataTypeId) ==
                 UA_TRUE) // enum is encoded as int32
        {

            bRet = PrintBuiltInType(UA_TYPES_INT32, pData, DataOffset,
                                    Indentation + 1, out);
        }
        else
        {
            // now it should be a built-in type
            // which should be available at ns0 UA_TYPES array
            const UA_DataType *pType = UA_findDataType(&DataTypeId);
            if (pType != 0)
            {
                bRet = PrintBuiltInType(pType->typeIndex, pData, DataOffset,
                                        Indentation + 1, out);
            }
            else
            {
                cout << "PrintVariant() Error: There's no datatype "
                        "information for DataType Id = '"
                     << DataTypeId << "' " << endl;
                bRet = UA_FALSE;
            }
        }
    }
    return bRet;
}

void PrintDataTypeInfo(const UA_DataType &Type, ostream &out)
{
    out << "\tDataType info:" << endl;
    out << "\t\tId = " << Type.typeId << endl;
    out << "\t\tName = " << Type.typeName << endl;
    out << "\t\tEncoding Id = " << Type.binaryEncodingId << endl;
    out << "\t\tIndex = " << Type.typeIndex << endl;
    out << "\t\tKind = " << Type.typeKind << endl;
    out << "\t\tPointer free = " << Type.pointerFree << endl;
    out << "\t\tOverlayable = " << Type.overlayable << endl;
    out << "\t\tMemSize = " << Type.memSize << endl;
    out << "\t\tMembersSize = " << Type.membersSize << endl;
    out << "\t\tMembers: = " << endl;
    for (size_t i = 0; i < Type.membersSize; i++)
    {
        out << "\t\t[" << endl;
        out << "\t\t\tName = " << Type.members[i].memberName << endl;
        out << "\t\t\tTypeIndex = " << Type.members[i].memberTypeIndex << endl;
        out << "\t\t\tNsZero = " << Type.members[i].namespaceZero << endl;
        out << "\t\t\tIsArray = " << Type.members[i].isArray << endl;
        out << "\t\t\tIsOptional = " << Type.members[i].isOptional << endl;
        out << "\t\t\tPadding = " << (UA_UInt32)Type.members[i].padding << endl;
        out << "\t\t]" << endl;
    }
}

// Note: DataTypeId is necessary for ExtensionObject, otherwise we can't get
// the type Id
UA_Boolean PrintValueAttribute(UA_Client *pClient, const UA_NodeId &DataTypeId,
                               const UA_Variant &Value, std::ostream &out)
{

    UA_Boolean bRet = UA_TRUE;

    out << "Value = {" << endl;

    // variants can be empty
    // then the pointer to the type description is null
    if (Value.type != 0)
    {
        PrintDataTypeInfo(*Value.type, out);
        out << "\tStorage Type = " << Value.storageType << endl;
        PrintArrayDimensions(Value.arrayDimensionsSize, Value.arrayDimensions,
                             1, out);
        out << "\tArray Length = " << Value.arrayLength << endl;
        out << "\tData = " << endl;

        size_t Offset = 0;
        size_t NestedStructureDepth = 0;
        size_t CurrentMember = 0;
        bRet = PrintVariant(pClient, Value.type->typeId, Value.type->members,
                            CurrentMember, NestedStructureDepth,
                            Value.arrayLength, Value.data, Offset, 1, out);
        if (bRet == UA_TRUE)
        {
            // check parsing offset and memsize
            size_t MemSize = Value.type->memSize *
                             ((Value.arrayLength == 0) ? 1 : Value.arrayLength);
            if (Offset != MemSize)
            {
                cout << "PrintValueAttribute Error: DataOffset error. "
                        "Offset = '"
                     << Offset << "' memSize = '" << Value.type->memSize << "'"
                     << endl;
                bRet = UA_FALSE;
            }
        }
    }
    else
    {
        out << "\tEmpty" << endl;
    }

    out << "}" << endl;
    return bRet;
}
