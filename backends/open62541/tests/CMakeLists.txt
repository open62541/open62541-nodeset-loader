add_executable(import check_nodeset_import.c)
target_link_libraries(import PRIVATE xmlImport open62541 backendOpen62541 ${CHECK_LIBRARIES})
add_test(NAME import_Test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ./import ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)

add_executable(namespaceZeroValues check_ns0_values.c)
target_link_libraries(namespaceZeroValues  PRIVATE xmlImport open62541 backendOpen62541 ${CHECK_LIBRARIES})
add_test(NAME namespaceZeroValues_Test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ./namespaceZeroValues ${PROJECT_SOURCE_DIR}/nodesets/namespaceZeroValues.xml)