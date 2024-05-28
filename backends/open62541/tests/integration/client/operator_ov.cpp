#include "operator_ov.h"
#include <open62541/types_generated.h>

#include <open62541/util.h>

using namespace std;

ostream &operator<<(ostream &os, const UA_NodeId &Id)
{
    UA_String str;
    UA_String_init(&str);
    if (UA_NodeId_print(&Id, &str) == UA_STATUSCODE_GOOD)
    {
        os << string((char *)str.data, str.length);
    }
    else
    {
        os << "error";
    }
    UA_String_clear(&str);
    return os;
}

std::ostream &operator<<(std::ostream &os, const UA_ExpandedNodeId &Id)
{
    os << Id.serverIndex << " : " << Id.namespaceUri << " : " << Id.nodeId;
    return os;
}

ostream &operator<<(ostream &os, const UA_String &Str)
{
    os << string((char *)Str.data, Str.length);
    return os;
}

std::ostream &operator<<(std::ostream &os, const UA_Guid &Guid)
{
    char Buffer[256] = {0};
    snprintf(Buffer, 256, UA_PRINTF_GUID_FORMAT, UA_PRINTF_GUID_DATA(Guid));
    os << Buffer;
    return os;
}

ostream &operator<<(ostream &os, const UA_NodeClass &NodeClass)
{
    switch (NodeClass)
    {
    case UA_NODECLASS_DATATYPE:
        os << "DATATYPE";
        break;
    case UA_NODECLASS_REFERENCETYPE:
        os << "REFERENCETYPE";
        break;
    case UA_NODECLASS_VARIABLETYPE:
        os << "VARIABLETYPE";
        break;
    case UA_NODECLASS_VARIABLE:
        os << "VARIABLE";
        break;
    case UA_NODECLASS_OBJECTTYPE:
        os << "OBJECTTYPE";
        break;
    case UA_NODECLASS_OBJECT:
        os << "OBJECT";
        break;
    case UA_NODECLASS_VIEW:
        os << "VIEW";
        break;
    case UA_NODECLASS_METHOD:
        os << "METHOD";
        break;
    case UA_NODECLASS_UNSPECIFIED:
        os << "UNSPECIFIED";
        break;
    default:
        os << "Error";
    }
    return os;
}

ostream &operator<<(ostream &os, const UA_QualifiedName &QualifiedName)
{
    os << "ns=" << QualifiedName.namespaceIndex << ";"
       << string((char *)QualifiedName.name.data, QualifiedName.name.length);
    return os;
}

ostream &operator<<(ostream &os, const UA_LocalizedText &LocalizedText)
{
    os << string((char *)LocalizedText.locale.data, LocalizedText.locale.length)
       << ":"
       << string((char *)LocalizedText.text.data, LocalizedText.text.length);
    return os;
}

std::ostream &operator<<(std::ostream &os,
                         const UA_DiagnosticInfo &DiagnosticInfo)
{
    // TODO
    os << "{" << endl;
    if (DiagnosticInfo.hasSymbolicId)
    {
        os << "\tSymbolic Id = " << DiagnosticInfo.symbolicId << " ";
    }
    if (DiagnosticInfo.hasNamespaceUri)
    {
        os << "\tNamespace URI = " << DiagnosticInfo.namespaceUri << " ";
    }
    if (DiagnosticInfo.hasLocalizedText)
    {
        os << "\tLocalized Text = " << DiagnosticInfo.localizedText << " ";
    }
    if (DiagnosticInfo.hasLocale)
    {
        os << "\tLocale = " << DiagnosticInfo.locale << " ";
    }
    if (DiagnosticInfo.hasAdditionalInfo)
    {
        os << "\tSymbolic Id = " << DiagnosticInfo.additionalInfo << " ";
    }
    if (DiagnosticInfo.hasInnerStatusCode)
    {
        os << "\tInner StatusCode = " << DiagnosticInfo.innerStatusCode << " ";
    }
    if (DiagnosticInfo.hasInnerDiagnosticInfo)
    {
        os << "\tInner Diagnostic Info = "
           << *DiagnosticInfo.innerDiagnosticInfo << " ";
    }
    os << "}" << endl;
    return os;
}

ostream &operator<<(ostream &os, const TReference &Reference)
{
    if ((Reference.pReferenceTypeId != 0) && (Reference.pTargetId != 0))
    {
        os << "| " << *Reference.pReferenceTypeId << " ; "
           << *Reference.pTargetId << " | ";
    }
    else
    {
        cout << "Error: Reference Id is null" << endl;
    }

    return os;
}
