add_executable(import import.c)
target_link_libraries(import PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME import_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./import ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)

add_executable(namespaceZeroValues ns0_values.c)
target_link_libraries(namespaceZeroValues PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME namespaceZeroValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./namespaceZeroValues ${CMAKE_CURRENT_SOURCE_DIR}/namespaceZeroValues.xml)

add_executable(valueRank valueRank.c)
target_link_libraries(valueRank PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME valueRank_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./valueRank ${CMAKE_CURRENT_SOURCE_DIR}/valueRank.xml)

add_executable(basicNodeClass basicNodeClass.c)
target_link_libraries(basicNodeClass PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME basicNodeClass_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./basicNodeClass ${CMAKE_CURRENT_SOURCE_DIR}/basicNodeClassTest.xml)

add_executable(extension extension.c)
target_link_libraries(extension PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME extension_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./extension ${CMAKE_CURRENT_SOURCE_DIR}/extension.xml)

add_executable(newHierachicalRef newHierachicalRef.c)
target_link_libraries(newHierachicalRef PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME newHierachicalRef_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND ./newHierachicalRef ${CMAKE_CURRENT_SOURCE_DIR}/newHierachicalRef.xml)