# build reference server with several nodesets using nodeset-compiler
message("Info: open62541 UA_NAMESPACE_ZERO=Full build option is required for this test")

# TODO: delete debug messages
message(${PROJECT_SOURCE_DIR})
message(${open62541_NODESET_DIR})

# Generate types and namespace for DI
ua_generate_nodeset_and_datatypes(
    NAME "integration_test_di"
    FILE_CSV "${open62541_NODESET_DIR}/DI/OpcUaDiModel.csv"
    FILE_BSD "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
    NAMESPACE_IDX 2
    FILE_NS "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    INTERNAL
)

# generate PLCopen namespace which is using DI
ua_generate_nodeset_and_datatypes(
    NAME "integration_test_plc"
    FILE_NS "${open62541_NODESET_DIR}/PLCopen/Opc.Ua.Plc.NodeSet2.xml" # PLCopen does not define custom types. Only generate the nodeset
    DEPENDS "integration_test_di" # PLCopen depends on the di nodeset, which must be generated before
    INTERNAL
)

# generate euromap83 namespace which is using DI
ua_generate_nodeset_and_datatypes(
    NAME "integration_test_euromap_83"
    FILE_CSV "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.csv"
    FILE_BSD "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.bsd"
    IMPORT_BSD "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    NAMESPACE_IDX 4
    FILE_NS "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
    DEPENDS "integration_test_di"
    INTERNAL
)

# generate euromap77 namespace which is using euromap83
ua_generate_nodeset_and_datatypes(
    NAME "integration_test_euromap_77"
    FILE_CSV "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.csv"
    FILE_BSD "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.bsd"
    IMPORT_BSD "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    NAMESPACE_IDX 5
    FILE_NS "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
    DEPENDS "integration_test_euromap_83"
    INTERNAL
)


add_executable(referenceServer server.cpp
    ${UA_NODESET_INTEGRATION_TEST_DI_SOURCES}  ${UA_TYPES_INTEGRATION_TEST_DI_SOURCES}
    ${UA_NODESET_INTEGRATION_TEST_PLC_SOURCES} ${UA_TYPES_INTEGRATION_TEST_PLC_SOURCES}
    ${UA_NODESET_INTEGRATION_TEST_EUROMAP_83_SOURCES} ${UA_TYPES_INTEGRATION_TEST_EUROMAP_83_SOURCES}
    ${UA_NODESET_INTEGRATION_TEST_EUROMAP_77_SOURCES} ${UA_TYPES_INTEGRATION_TEST_EUROMAP_77_SOURCES})
target_link_libraries(referenceServer PRIVATE open62541::open62541)
target_include_directories(referenceServer PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

#add_dependencies(referenceServer open62541-generator-integration-ns)
# what does add_dependencies??
