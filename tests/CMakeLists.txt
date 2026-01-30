add_executable(nodeContainer NodeContainer.c ${CMAKE_CURRENT_SOURCE_DIR}/../src/Node.c)
target_include_directories(nodeContainer PRIVATE ${CHECK_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(nodeContainer PRIVATE ${CHECK_LIBRARIES} ${PTHREAD_LIB} coverageLib open62541::open62541)
add_test(NAME nodeContainer_Test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND nodeContainer ${CMAKE_CURRENT_LIST_DIR})

add_executable(allocator allocator.c ${CMAKE_CURRENT_SOURCE_DIR}/../src/CharAllocator.c)
target_include_directories(allocator PRIVATE ${CHECK_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(allocator PRIVATE ${CHECK_LIBRARIES} ${PTHREAD_LIB} coverageLib open62541::open62541)
add_test(NAME allocatorTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND allocator ${CMAKE_CURRENT_LIST_DIR})

#these tests are simple loading nodesets and dumping it to stdout
add_test(NAME import_testNodeset WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)
add_test(NAME import_Nodeset2 WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml)
add_test(NAME import_DI WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Di.NodeSet2.xml)

#add_test(NAME import_PLCOpen WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.NodeSet2.xml 
#                            ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Di.NodeSet2.xml
#                            ${PROJECT_SOURCE_DIR}/nodesets/Opc.Ua.Plc.NodeSet2.xml)

