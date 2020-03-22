#add_executable(check_sort check_sort.cpp)
#target_include_directories(check_sort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
#target_link_libraries(check_sort CONAN_PKG::gtest xmlImport)
#add_test(NAME sortTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND check_sort ${CMAKE_CURRENT_LIST_DIR})

add_test(NAME import_testNodeset WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND ./parserDemo ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)
add_test(NAME import_Nodeset2 WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml)
add_test(NAME import_DI WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin 
    COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/DI/Opc.Ua.Di.NodeSet2.xml)
add_test(NAME import_PLCOpen WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND parserDemo ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml 
                                ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                                ${PROJECT_SOURCE_DIR}/nodesets/UA-Nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml)