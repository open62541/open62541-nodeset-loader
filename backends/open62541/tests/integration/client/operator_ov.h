#ifndef _OPERATOR_OV_H
#define _OPERATOR_OV_H

#include "browse_utils.h"
#include <iostream>
#include <open62541/types_generated.h>

std::ostream &operator<<(std::ostream &os, const UA_NodeId &Id);
std::ostream &operator<<(std::ostream &os, const UA_ExpandedNodeId &Id);
std::ostream &operator<<(std::ostream &os, const UA_String &Str);
std::ostream &operator<<(std::ostream &os, const UA_Guid &Guid);
std::ostream &operator<<(std::ostream &os, const UA_NodeClass &NodeClass);
std::ostream &operator<<(std::ostream &os,
                         const UA_QualifiedName &QualifiedName);
std::ostream &operator<<(std::ostream &os,
                         const UA_LocalizedText &LocalizedText);
std::ostream &operator<<(std::ostream &os,
                         const UA_DiagnosticInfo &DiagnosticInfo);
std::ostream &operator<<(std::ostream &os, const TReference &Reference);

#endif // _OPERATOR_OV_H