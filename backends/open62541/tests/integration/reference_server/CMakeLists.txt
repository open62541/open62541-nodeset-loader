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

add_executable(referenceServer server.cpp
    ${UA_NODESET_INTEGRATION_TEST_DI_SOURCES}  ${UA_TYPES_INTEGRATION_TEST_DI_SOURCES}
    ${UA_NODESET_INTEGRATION_TEST_PLC_SOURCES} ${UA_TYPES_INTEGRATION_TEST_PLC_SOURCES})
target_link_libraries(referenceServer PRIVATE open62541::open62541)
target_include_directories(referenceServer PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

#add_dependencies(referenceServer open62541-generator-integration-ns)
# what does add_dependencies??
