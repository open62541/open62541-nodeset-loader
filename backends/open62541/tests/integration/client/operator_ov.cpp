#include "operator_ov.h"

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

ostream &operator<<(ostream &os, const UA_String &Str)
{
    os << string((char *)Str.data, Str.length);
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