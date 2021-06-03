#ifndef _COMMON_DEFS_H
#define _COMMON_DEFS_H

#include <open62541/nodeids.h>
#include <open62541/types.h>

/*****************************************************************************/
// constants
static const UA_NodeId HasPropertyId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
static const UA_NodeId HasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
static const UA_NodeId HierarchicalReferenceId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
static const UA_NodeId HasTypeDefinitionId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
static const UA_NodeId HasModellingRuleId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);
static const UA_NodeId HasEncodingId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_HASENCODING);
static const UA_NodeId StructureId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
static const UA_NodeId EnumerationId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_ENUMERATION);
static const UA_NodeId OptionSetId = UA_NODEID_NUMERIC(0, UA_NS0ID_OPTIONSET);
static const UA_NodeId OptionSetValuesId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_OPTIONSETVALUES);
static const UA_NodeId UnsignedIntegerId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_UINTEGER);
static const UA_NodeId DataTypeDefinitionId =
    UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPEDEFINITION);

static const UA_QualifiedName PropertyBrowseNameOptionSetValues =
    UA_QUALIFIEDNAME(0, (char *)"OptionSetValues");

#endif // _COMMON_DEFS_H