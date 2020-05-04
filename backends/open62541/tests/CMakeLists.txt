add_executable(import import.c)
target_link_libraries(import PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME import_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND import ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)

add_executable(namespaceZeroValues namespaceZeroValues.c)
target_link_libraries(namespaceZeroValues PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME namespaceZeroValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND namespaceZeroValues ${CMAKE_CURRENT_SOURCE_DIR}/namespaceZeroValues.xml)

add_executable(customTypesWithValues customTypesWithValues.c)
target_link_libraries(customTypesWithValues PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME customTypesWithValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND customTypesWithValues ${CMAKE_CURRENT_SOURCE_DIR}/customTypesWithValues.xml)

add_executable(valueRank valueRank.c)
target_link_libraries(valueRank PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME valueRank_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND valueRank ${CMAKE_CURRENT_SOURCE_DIR}/valueRank.xml)

add_executable(basicNodeClasses basicNodeClasses.c)
target_link_libraries(basicNodeClasses PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME basicNodeClasses_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND basicNodeClasses ${CMAKE_CURRENT_SOURCE_DIR}/basicNodeClasses.xml)

add_executable(extension extension.c)
target_link_libraries(extension PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME extension_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND extension ${CMAKE_CURRENT_SOURCE_DIR}/extension.xml)

add_executable(newHierachicalRef newHierachicalRef.c)
target_link_libraries(newHierachicalRef PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME newHierachicalRef_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND newHierachicalRef ${CMAKE_CURRENT_SOURCE_DIR}/newHierachicalRef.xml)

add_executable(references references.c)
target_link_libraries(references PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME references_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND references ${CMAKE_CURRENT_SOURCE_DIR}/references.xml)

add_executable(multipleNamespaces multipleNamespaces.c)
target_link_libraries(multipleNamespaces PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME multipleNamespaces_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND multipleNamespaces ${CMAKE_CURRENT_SOURCE_DIR}/multipleNamespaces.xml)

add_subdirectory(dataTypeImport)