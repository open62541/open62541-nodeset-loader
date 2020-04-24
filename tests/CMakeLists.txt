add_executable(sort sort.c)
target_include_directories(sort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(sort PRIVATE NodesetLoader ${CHECK_LIBRARIES} pthread)
add_test(NAME sort_Test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND sort ${CMAKE_CURRENT_LIST_DIR})

SET(STDOUT_BINARY_DIR ${CMAKE_BINARY_DIR}/backends/stdout/examples)

#these tests are simple loading nodesets and dumping it to stdout
add_test(NAME import_testNodeset WORKING_DIRECTORY ${STDOUT_BINARY_DIR} COMMAND ./parserDemo ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)
add_test(NAME import_Nodeset2 WORKING_DIRECTORY ${STDOUT_BINARY_DIR} COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml)
add_test(NAME import_DI WORKING_DIRECTORY ${STDOUT_BINARY_DIR}
    COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Di.NodeSet2.xml)
add_test(NAME import_PLCOpen WORKING_DIRECTORY ${STDOUT_BINARY_DIR} COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml 
                                ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Di.NodeSet2.xml
                                ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Plc.NodeSet2.xml)