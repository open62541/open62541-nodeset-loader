#include "utils.h"
#include "browse_utils.h"
#include "common_defs.h"
#include "operator_ov.h"
#include "value_utils.h"
#include <iostream>
#include <open62541/client_highlevel.h>
#include <open62541/util.h>
#include <string>
#include <vector>

using namespace std;

/*****************************************************************************/
// print functions:

/*****************************************************************************/
UA_Boolean PrintBaseNodeAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    // M: NodeId
    out << "Id = " << Id << " ";

    // M: NodeClass
    UA_NodeClass nodeClass;
    if (UA_Client_readNodeClassAttribute(pClient, Id, &nodeClass) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << nodeClass << endl;

    // M: BrowseName
    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    if (UA_Client_readBrowseNameAttribute(pClient, Id, &browseName) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "BrowseName = " << browseName << endl;
    UA_QualifiedName_clear(&browseName);

    // M: DisplayName
    UA_LocalizedText displayName;
    UA_LocalizedText_init(&displayName);
    if (UA_Client_readDisplayNameAttribute(pClient, Id, &displayName) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "DisplayName = " << displayName << endl;
    UA_LocalizedText_clear(&displayName);

    // O: Description
    UA_LocalizedText description;
    UA_LocalizedText_init(&description);
    if (UA_Client_readDescriptionAttribute(pClient, Id, &description) ==
        UA_STATUSCODE_GOOD)
    {
        out << "Description = " << description << endl;
    }
    UA_LocalizedText_clear(&description);

    // O: WriteMask
    UA_UInt32 writeMask = 0;
    if (UA_Client_readWriteMaskAttribute(pClient, Id, &writeMask) ==
        UA_STATUSCODE_GOOD)
    {
        out << "WriteMask = " << writeMask << endl;
    }

    // O: UserWriteMask
    UA_UInt32 userWriteMask = 0;
    if (UA_Client_readWriteMaskAttribute(pClient, Id, &userWriteMask) ==
        UA_STATUSCODE_GOOD)
    {
        out << "UserWriteMask = " << userWriteMask << endl;
    }

    // O: RolePermissions // TODO: there's no client read function?

    // O: UserRolePermissions // TODO: there's no client read function?

    // O: AccessRestrictions // TODO: there's no client read function?

    return UA_TRUE;
}

/*****************************************************************************/
// ReferenceType

UA_Boolean PrintReferenceTypeAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                        ofstream &out)
{
    // M: IsAbstract
    UA_Boolean isAbstract = UA_FALSE;
    if (UA_Client_readIsAbstractAttribute(pClient, Id, &isAbstract) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "IsAbstract = " << ((isAbstract) ? "true" : "false") << endl;

    // M: IsSymmetric
    UA_Boolean isSymmetric = UA_FALSE;
    if (UA_Client_readSymmetricAttribute(pClient, Id, &isSymmetric) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "IsSymmetric = " << ((isSymmetric) ? "true" : "false") << endl;

    // O: InverseName
    UA_LocalizedText inverseName;
    UA_LocalizedText_init(&inverseName);
    if (UA_Client_readInverseNameAttribute(pClient, Id, &inverseName) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "InverseName = " << inverseName << endl;
    UA_LocalizedText_clear(&inverseName);

    return UA_TRUE;
}

UA_Boolean PrintReferenceTypeReferences(UA_Client *pClient, const UA_NodeId &Id,
                                        ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        /* HasSubtype References and HasProperty References are the only
           ReferenceTypes that may be used with ReferenceType Nodes as
           SourceNode. ReferenceType Nodes shall not be the SourceNode of other
           types of References. */

        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;

            // check references type
            if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                &HasPropertyId) == UA_TRUE)
            {
                // 0..* HasProperty: shall only refer to nodes of type VARIABLE
                // nodeclass

                UA_NodeClass nodeClass;
                if (UA_Client_readNodeClassAttribute(
                        pClient, *ReferencesVec[i].pTargetId, &nodeClass) ==
                    UA_STATUSCODE_GOOD)
                {
                    if (nodeClass != UA_NODECLASS_VARIABLE)
                    {
                        cout << "Error PrintReferenceTypeReferences: "
                                "HasProperty reference of ReferenceType node "
                                "must be of nodeclass VARIABLE"
                             << endl;
                    }

                    // standard HasProperty references:
                    // O: NodeVersion
                }
                else
                {
                    cout << "Error PrintReferenceTypeReferences: reading "
                            "nodeclass attribute failed"
                         << endl;
                }
            }
            else if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                     &HasSubtypeId) ==
                     UA_FALSE) // there shall not be any other reference than
                               // HasSubType: 0..* HasSubtype
            {

                cout << "Error PrintReferenceTypeReferences: ReferenceType "
                        "node has an invalid reference"
                     << endl;
            }
        }
    }
    FreeReferencesVec(ReferencesVec);

    return ret;
}

/*****************************************************************************/
// View

UA_Boolean PrintViewAttributes(UA_Client *pClient, const UA_NodeId &Id,
                               ofstream &out)
{
    // M: ContainsNoLoops
    UA_Boolean containsNoLoops = UA_FALSE;
    if (UA_Client_readContainsNoLoopsAttribute(pClient, Id, &containsNoLoops) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "ContainsNoLoops = " << ((containsNoLoops) ? "true" : "false")
        << endl;

    // M: EventNotifier
    UA_Byte eventNotifier = UA_FALSE;
    if (UA_Client_readEventNotifierAttribute(pClient, Id, &eventNotifier) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "EventNotifier = " << (UA_UInt32)eventNotifier << endl;

    return UA_TRUE;
}

// TODO: not sure how to print view references
// as these are hierarchical references ...
UA_Boolean PrintViewReferences(UA_Client *pClient, const UA_NodeId &Id,
                               ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << ReferencesVec[i];

            /* references of a view can be of type
                - 0..* HasProperty
                - 0..* HierachicalReference
            */

            // check if reference type is either a HasProperty type or a subtype
            // of HierarchicalReference
            if ((UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                 &HasPropertyId) == UA_FALSE) ||
                (IsSubType(pClient, HierarchicalReferenceId,
                           *ReferencesVec[i].pReferenceTypeId) == UA_FALSE))
            {
                cout << "Error PrintViewReferences: ReferenceType "
                        "node has an invalid reference"
                     << endl;
            }
        }
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// Objects

UA_Boolean PrintObjectAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                 ofstream &out)
{
    // M: EventNotifier
    UA_Byte eventNotifier = UA_FALSE;
    if (UA_Client_readEventNotifierAttribute(pClient, Id, &eventNotifier) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "EventNotifier = " << (UA_UInt32)eventNotifier << endl;

    return UA_TRUE;
}

UA_Boolean PrintObjectReferences(UA_Client *pClient, const UA_NodeId &Id,
                                 ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        UA_UInt32 NoOfHasTypeDefinitonReferences = 0;
        UA_UInt32 NoOfHasModellingRuleReferences = 0;
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;

            if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                &HasTypeDefinitionId) == UA_TRUE)
            {
                NoOfHasTypeDefinitonReferences++;
            }
            else if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                     &HasModellingRuleId) == UA_TRUE)
            {
                NoOfHasModellingRuleReferences++;
            }
        }

        // check that Object has exactly 1 HasTypeDefinition reference
        if (NoOfHasTypeDefinitonReferences != 1)
        {
            cout << "Error PrintObjectsReferences: object does not have "
                    "exactly 1 HasTypeDefinition reference"
                 << endl;
        }

        if (NoOfHasModellingRuleReferences > 1)
        {
            cout << "Error PrintObjectsReferences: object does have more than "
                    "1 ModellingRule reference"
                 << endl;
        }

        // objects may contain other references as well, so we do no more
        // additional checks here
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// ObjectType

UA_Boolean PrintObjectTypeAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                     ofstream &out)
{
    // M: IsAbstract
    UA_Boolean isAbstract = UA_FALSE;
    if (UA_Client_readIsAbstractAttribute(pClient, Id, &isAbstract) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "IsAbstract = " << ((isAbstract) ? "true" : "false") << endl;

    return UA_TRUE;
}

UA_Boolean PrintObjectTypeReferences(UA_Client *pClient, const UA_NodeId &Id,
                                     ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;
        }
        // objects may contain other references as well, so we do no more
        // additional checks here
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// Variable

UA_Boolean PrintVariableAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    // M: DataType
    UA_NodeId dataType;
    UA_NodeId_init(&dataType);
    if (UA_Client_readDataTypeAttribute(pClient, Id, &dataType) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "DataType = " << dataType << endl;

    // M: Value
    UA_Variant Value;
    UA_Variant_init(&Value);
    if (UA_Client_readValueAttribute(pClient, Id, &Value) != UA_STATUSCODE_GOOD)
    {
        cout << "PrintVariableAttributes() Error: Reading value attribute of "
                "node Id = "
             << Id << " failed " << endl;
        UA_NodeId_clear(&dataType);
        return UA_FALSE;
    }
    UA_Boolean tmpRet = PrintValueAttribute(Value, out);
    UA_Variant_clear(&Value);
    UA_NodeId_clear(&dataType);
    if (tmpRet == UA_FALSE)
    {
        cout << "PrintVariableAttributes() Error: Print value of "
                "node Id = "
             << Id << " failed " << endl;
        return UA_FALSE;
    }

    // M: ValueRank
    UA_Int32 valueRank = 0;
    if (UA_Client_readValueRankAttribute(pClient, Id, &valueRank) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "ValueRank = " << valueRank << endl;

    // O: ArrayDimensions
    size_t arrayDimSize = 0;
    UA_UInt32 *pArrayDimensions = 0;
    if (UA_Client_readArrayDimensionsAttribute(pClient, Id, &arrayDimSize,
                                               &pArrayDimensions) ==
        UA_STATUSCODE_GOOD)
    {
        PrintArrayDimensions(arrayDimSize, pArrayDimensions, 0, out);
        UA_free(pArrayDimensions);
        pArrayDimensions = 0;
    }

    // M: AccessLevel
    UA_AccessLevelType accessLevel = 0;
    if (UA_Client_readAccessLevelAttribute(pClient, Id, &accessLevel) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "AccessLevel = " << (UA_UInt32)accessLevel << endl;

    // M: UserAccessLevel
    UA_AccessLevelType UserAccessLevel = 0;
    if (UA_Client_readUserAccessLevelAttribute(pClient, Id, &UserAccessLevel) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "UserAccessLevel = " << (UA_UInt32)UserAccessLevel << endl;

    // O: MinimumSamplingInterval
    UA_Duration minSamplingInterval = 0.0;
    if (UA_Client_readMinimumSamplingIntervalAttribute(
            pClient, Id, &minSamplingInterval) == UA_STATUSCODE_GOOD)
    {
        out << "MinimumSamplingInterval = " << minSamplingInterval << endl;
    }

    // M: Historizing
    UA_Boolean historizing = UA_FALSE;
    if (UA_Client_readHistorizingAttribute(pClient, Id, &historizing) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "Historizing = " << ((historizing) ? "true" : "false") << endl;

    // O: AccessLevelEx // TODO: there's no client read function

    return UA_TRUE;
}

UA_Boolean PrintVariableReferences(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        UA_UInt32 NoOfHasTypeDefinitonReferences = 0;
        UA_UInt32 NoOfHasModellingRuleReferences = 0;
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;

            if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                &HasTypeDefinitionId) == UA_TRUE)
            {
                NoOfHasTypeDefinitonReferences++;
            }
            else if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                     &HasModellingRuleId) == UA_TRUE)
            {
                NoOfHasModellingRuleReferences++;
            }
        }

        // check that variable has exactly 1 HasTypeDefinition reference
        if (NoOfHasTypeDefinitonReferences != 1)
        {
            cout << "Error PrintVariableReferences: variable does not have "
                    "exactly 1 HasTypeDefinition reference"
                 << endl;
        }

        if (NoOfHasModellingRuleReferences > 1)
        {
            cout << "Error PrintVariableReferences: variable does have "
                    "more than "
                    "1 ModellingRule reference"
                 << endl;
        }

        // variables may contain other references as well, so we do no more
        // additional checks here
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// VariableType

UA_Boolean PrintVariableTypeAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                       ofstream &out)
{
    // M: DataType
    UA_NodeId dataType;
    UA_NodeId_init(&dataType);
    if (UA_Client_readDataTypeAttribute(pClient, Id, &dataType) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "DataType = " << dataType << endl;

    // O: Value
    UA_Variant Value;
    UA_Variant_init(&Value);
    if (UA_Client_readValueAttribute(pClient, Id, &Value) == UA_STATUSCODE_GOOD)
    {
        if (PrintValueAttribute(Value, out) == UA_FALSE)
        {
            cout << "PrintVariableTypeAttributes() Error: Print value of "
                    "node Id = "
                 << Id << " failed " << endl;
            UA_Variant_clear(&Value);
            UA_NodeId_clear(&dataType);
            return UA_FALSE;
        }
    }
    UA_Variant_clear(&Value);
    UA_NodeId_clear(&dataType);

    // M: ValueRank
    UA_Int32 valueRank = 0;
    if (UA_Client_readValueRankAttribute(pClient, Id, &valueRank) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "ValueRank = " << valueRank << endl;

    // O: ArrayDimensions
    size_t arrayDimSize = 0;
    UA_UInt32 *pArrayDimensions = 0;
    if (UA_Client_readArrayDimensionsAttribute(pClient, Id, &arrayDimSize,
                                               &pArrayDimensions) ==
        UA_STATUSCODE_GOOD)
    {
        PrintArrayDimensions(arrayDimSize, pArrayDimensions, 0, out);
        UA_free(pArrayDimensions);
        pArrayDimensions = 0;
    }

    // M: IsAbstract
    UA_Boolean isAbstract = UA_FALSE;
    if (UA_Client_readIsAbstractAttribute(pClient, Id, &isAbstract) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "IsAbstract = " << ((isAbstract) ? "true" : "false") << endl;

    return UA_TRUE;
}

UA_Boolean PrintVariableTypeReferences(UA_Client *pClient, const UA_NodeId &Id,
                                       ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;
        }
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// Method

UA_Boolean PrintMethodAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                 ofstream &out)
{
    // M: Executable
    UA_Boolean executable;
    if (UA_Client_readExecutableAttribute(pClient, Id, &executable) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "Executable = " << executable << endl;

    UA_Boolean userExecutable;
    if (UA_Client_readExecutableAttribute(pClient, Id, &userExecutable) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "UserExecutable = " << userExecutable << endl;

    return UA_TRUE;
}

UA_Boolean PrintMethodReferences(UA_Client *pClient, const UA_NodeId &Id,
                                 ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        UA_UInt32 NoOfHasModellingRuleReferences = 0;
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;

            if (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                &HasModellingRuleId) == UA_TRUE)
            {
                NoOfHasModellingRuleReferences++;
            }
        }
        if (NoOfHasModellingRuleReferences > 1)
        {
            cout << "Error PrintMethodReferences: method does have "
                    "more than "
                    "1 ModellingRule reference"
                 << endl;
        }

        // methods may contain other references as well, so we do no more
        // additional checks here
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
// DataType

void PrintStructureDefinition(const UA_StructureDefinition &StructDefinition,
                              ofstream &out)
{
    out << "{ Structure: " << endl;

    out << "\tBaseDataType = " << StructDefinition.baseDataType << endl;
    out << "\tEncodingId = " << StructDefinition.defaultEncodingId << endl;
    out << "\tStructureType = " << StructDefinition.structureType << endl;
    out << "\tFieldsSize = " << StructDefinition.fieldsSize << endl;

    UA_StructureField *pField = 0;
    for (size_t i = 0; i < StructDefinition.fieldsSize; i++)
    {
        out << "\t[ " << endl;
        pField = &StructDefinition.fields[i];
        out << "\t\tName = " << pField->name << endl;
        out << "\t\tDescription = " << pField->description << endl;
        out << "\t\tDataType = " << pField->dataType << endl;
        out << "\t\tValueRank = " << pField->valueRank << endl;
        PrintArrayDimensions(pField->arrayDimensionsSize,
                             pField->arrayDimensions, 2, out);
        out << "\t\tMaxStringLength = " << pField->maxStringLength << endl;
        out << "\t\tIsOptional = " << pField->isOptional << endl;
        out << "\t] " << endl;
    }
    out << "}" << endl;
}

void PrintEnumDefinition(const UA_EnumDefinition &EnumDefinition, ofstream &out)
{
    out << "{ Enum: " << endl;
    out << "\tFieldsSize = " << EnumDefinition.fieldsSize << endl;

    UA_EnumField *pField = 0;
    for (size_t i = 0; i < EnumDefinition.fieldsSize; i++)
    {
        out << "\t[ ";
        pField = &EnumDefinition.fields[i];
        out << "\t\tName = " << pField->name << endl;
        out << "\t\tValue = " << pField->value << endl;
        out << "\t\tDisplayName = " << pField->displayName << endl;
        out << "\t\tDescription = " << pField->description << endl;
        out << "\t]" << endl;
    }
    out << "}" << endl;
}

UA_Boolean PrintDataTypeDefinition(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    /*
    The Attribute is mandatory for DataTypes derived from Structure and
    Union. For such DataTypes, the Attribute contains a structure of the
    DataType StructureDefinition.

    The Attribute is mandatory for DataTypes derived from Enumeration,
    OptionSet and subtypes of UInteger representing an OptionSet. For
    such DataTypes, the Attribute contains a structure of the DataType
    EnumDefinition.
    */

    UA_Boolean bRet = UA_TRUE;

    UA_Boolean IsDerivedFromStructure = IsSubType(pClient, StructureId, Id);
    UA_Boolean IsDerivedFromEnumeration = IsSubType(pClient, EnumerationId, Id);
    UA_Boolean IsDerivedFromOptionSet = IsSubType(pClient, OptionSetId, Id);

    // check for derived UInteger type with Property OptionSetValues
    UA_Boolean IsUIntegerOptionSet = UA_FALSE;
    if (IsSubType(pClient, UnsignedIntegerId, Id))
    {
        // search for HasProperty OptionSetValues reference
        TReferenceVec ReferenceVec;
        if (BrowseReferences(pClient, Id, ReferenceVec) == UA_TRUE)
        {
            for (size_t i = 0; i < ReferenceVec.size(); i++)
            {
                if (UA_NodeId_equal(ReferenceVec[i].pReferenceTypeId,
                                    &HasPropertyId))
                {
                    UA_QualifiedName BrowseName;
                    UA_QualifiedName_init(&BrowseName);
                    if (UA_Client_readBrowseNameAttribute(
                            pClient, *ReferenceVec[i].pTargetId, &BrowseName) !=
                        UA_STATUSCODE_GOOD)
                    {
                        cout << "Error PrintDataTypeDefinition(): Reading "
                                "OptionSet HasProperty browsename failed"
                             << endl;
                        bRet = UA_FALSE;
                    }
                    else
                    {
                        if (UA_QualifiedName_equal(
                                &PropertyBrowseNameOptionSetValues,
                                &BrowseName) == UA_TRUE)
                        {
                            IsUIntegerOptionSet = UA_TRUE;
                        }
                    }
                    UA_QualifiedName_clear(&BrowseName);
                }
            }
            FreeReferencesVec(ReferenceVec);
        }
        else
        {
            cout << "Error PrintDataTypeDefinition(): '" << Id
                 << "' BrowseReferences failed" << endl;
            return UA_FALSE;
        }
    }

    if (IsDerivedFromStructure || IsDerivedFromEnumeration ||
        IsDerivedFromOptionSet || IsUIntegerOptionSet)
    {
        // attribute DataTypeDefinition is mandatory
        out << "DataTypeDefinition = ";

        UA_ReadRequest rReq;
        UA_ReadRequest_init(&rReq);
        rReq.nodesToReadSize = 1;
        rReq.nodesToRead = UA_ReadValueId_new();
        UA_ReadValueId_init(rReq.nodesToRead);
        UA_NodeId_copy(&Id, &rReq.nodesToRead->nodeId);
        rReq.nodesToRead->attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;

        UA_ReadResponse rResp;
        UA_ReadResponse_init(&rResp);
        rResp = UA_Client_Service_read(pClient, rReq);
        if (rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        {
            if ((rResp.resultsSize != 1) ||
                (rResp.results[0].hasValue == UA_FALSE))
            {
                /* TODO: it seems that the DataTypeDefinition attribute is
                   not supported for a lot of types. E.g. StructureDefiniton
                   itself, Enumerations, etc. As this is the case for the
                   referenceServer too, we can't create an error here ... So
                   we only print an Info
                */
                cout << "Info PrintDataTypeDefinition: '" << Id
                     << "' has no DataTypeDefiniton attribute, although it "
                        "should have ..."
                     << endl;
            }
            else
            {
                if ((IsDerivedFromStructure) &&
                    (UA_Variant_hasScalarType(
                        &rResp.results[0].value,
                        &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])))
                {
                    PrintStructureDefinition(
                        *((UA_StructureDefinition *)rResp.results[0]
                              .value.data),
                        out);
                }
                else if (IsDerivedFromEnumeration || IsDerivedFromOptionSet ||
                         IsUIntegerOptionSet ||
                         UA_Variant_hasScalarType(
                             &rResp.results[0].value,
                             &UA_TYPES[UA_TYPES_ENUMDEFINITION]))
                {
                    PrintEnumDefinition(
                        *((UA_EnumDefinition *)rResp.results[0].value.data),
                        out);
                }
                else
                {
                    cout << "Error PrintDataTypeDefinition: '" << Id
                         << "' DataTypeDefiniton "
                            "attribute is wrong"
                         << endl;
                    bRet = UA_FALSE;
                }
            }
        }
        else
        {
            cout << "Error PrintDataTypeDefinition:  '" << Id
                 << "' read service result is bad" << endl;
            bRet = UA_FALSE;
        }
        UA_ReadRequest_clear(&rReq);
        UA_ReadResponse_clear(&rResp);
        out << endl;
    }

    return bRet;
}

UA_Boolean PrintDataTypeAttributes(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    // M: IsAbstract
    UA_Boolean isAbstract;
    if (UA_Client_readIsAbstractAttribute(pClient, Id, &isAbstract) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }
    out << "IsAbstract = " << ((isAbstract) ? "true" : "false") << endl;

    // O: DataTypeDefinition
    if (PrintDataTypeDefinition(pClient, Id, out) == UA_FALSE)
    {
        return UA_FALSE;
    }

    return UA_TRUE;
}

UA_Boolean PrintDataTypeReferences(UA_Client *pClient, const UA_NodeId &Id,
                                   ofstream &out)
{
    out << "References: " << endl;
    TReferenceVec ReferencesVec;
    UA_Boolean ret = BrowseReferences(pClient, Id, ReferencesVec);
    if (ret == UA_TRUE)
    {
        for (size_t i = 0; i < ReferencesVec.size(); i++)
        {
            out << "\t" << ReferencesVec[i] << endl;

            if ((UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                 &HasPropertyId) == UA_FALSE) &&
                (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                 &HasSubtypeId) == UA_FALSE) &&
                (UA_NodeId_equal(ReferencesVec[i].pReferenceTypeId,
                                 &HasEncodingId) == UA_FALSE))
            {
                cout << "Error PrintDataTypeReferences: Id = '" << Id
                     << "' : DataType node has an invalid reference type '"
                     << *ReferencesVec[i].pReferenceTypeId << "'" << endl;
                ret = UA_FALSE;
            }
        }
    }
    FreeReferencesVec(ReferencesVec);
    return ret;
}

/*****************************************************************************/
UA_Boolean PrintNode(UA_Client *pClient, const UA_NodeId &Id, ofstream &out)
{
    UA_Boolean ret = PrintBaseNodeAttributes(pClient, Id, out);

    UA_NodeClass nodeClass;
    if (UA_Client_readNodeClassAttribute(pClient, Id, &nodeClass) !=
        UA_STATUSCODE_GOOD)
    {
        return UA_FALSE;
    }

    switch (nodeClass)
    {
    case UA_NODECLASS_DATATYPE:
        ret &= PrintDataTypeAttributes(pClient, Id, out);
        ret &= PrintDataTypeReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        ret &= PrintReferenceTypeAttributes(pClient, Id, out);
        ret &= PrintReferenceTypeReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        ret &= PrintVariableTypeAttributes(pClient, Id, out);
        ret &= PrintVariableTypeReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_VARIABLE:
        ret &= PrintVariableAttributes(pClient, Id, out);
        ret &= PrintVariableReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        ret &= PrintObjectTypeAttributes(pClient, Id, out);
        ret &= PrintObjectTypeReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_OBJECT:
        ret &= PrintObjectAttributes(pClient, Id, out);
        ret &= PrintObjectReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_VIEW:
        ret &= PrintViewAttributes(pClient, Id, out);
        // ret &= PrintViewReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_METHOD:
        ret &= PrintMethodAttributes(pClient, Id, out);
        ret &= PrintMethodReferences(pClient, Id, out);
        break;
    case UA_NODECLASS_UNSPECIFIED:
        cout << "Error PrintNode: nodeclass is 'unspecified'" << endl;
        ret &= UA_FALSE;
        break;
    default:
        cout << "Error PrintNode: nodeclass is unknown" << endl;
        ret &= UA_FALSE;
        break;
    }

    if (ret == UA_FALSE)
    {
        cout << "PrintNode() Error: at NodeId = " << Id << endl;
        out << "PrintNode() Error: at NodeId = " << Id << endl;
    }

    out << endl;
    return ret;
}
