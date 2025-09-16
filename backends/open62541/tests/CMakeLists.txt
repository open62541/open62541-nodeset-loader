add_executable(import import.c)
target_include_directories(import PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(import PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME import_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND import ${PROJECT_SOURCE_DIR}/nodesets/testNodeset100nodes.xml)

add_executable(namespaceZeroValues namespaceZeroValues.c)
target_include_directories(namespaceZeroValues PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(namespaceZeroValues PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME namespaceZeroValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND namespaceZeroValues ${CMAKE_CURRENT_SOURCE_DIR}/namespaceZeroValues.xml)

add_executable(customTypesWithValues customTypesWithValues.c)
target_include_directories(customTypesWithValues PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(customTypesWithValues PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME customTypesWithValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND customTypesWithValues ${CMAKE_CURRENT_SOURCE_DIR}/customTypesWithValues.xml)

add_executable(subDataTypes subDataTypes.c)
target_include_directories(subDataTypes PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(subDataTypes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME subDataTypes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND subDataTypes ${CMAKE_CURRENT_SOURCE_DIR}/subDataTypes.xml)

add_executable(valueRank valueRank.c)
target_include_directories(valueRank PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(valueRank PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME valueRank_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND valueRank ${CMAKE_CURRENT_SOURCE_DIR}/valueRank.xml)

add_executable(basicNodeClasses basicNodeClasses.c)
target_include_directories(basicNodeClasses PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(basicNodeClasses PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME basicNodeClasses_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND basicNodeClasses ${CMAKE_CURRENT_SOURCE_DIR}/basicNodeClasses.xml)

add_executable(issue_201 issue_201.c)
target_include_directories(issue_201 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(issue_201 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME issue_201_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND issue_201 ${CMAKE_CURRENT_SOURCE_DIR}/issue_201.xml)

add_executable(issue_241 issue_241.c)
target_include_directories(issue_241 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(issue_241 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME issue_241_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND issue_241 ${CMAKE_CURRENT_SOURCE_DIR}/issue_241.xml)

add_executable(stringNodeId_issue_224 stringNodeId_issue_224.c)
target_include_directories(stringNodeId_issue_224 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(stringNodeId_issue_224 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME stringNodeId_issue_224
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
COMMAND stringNodeId_issue_224 ${CMAKE_CURRENT_SOURCE_DIR}/stringNodeId_issue_224.xml)

add_executable(orderingStringNodeIds orderingStringNodeIds.c)
target_include_directories(orderingStringNodeIds PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(orderingStringNodeIds PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME orderingStringNodeIds
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
COMMAND orderingStringNodeIds ${CMAKE_CURRENT_SOURCE_DIR}/orderingStringNodeIds.xml)

add_executable(cornerCases cornerCases.c)
target_include_directories(cornerCases PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(cornerCases PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME cornerCases_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND cornerCases ${CMAKE_CURRENT_SOURCE_DIR}/cornerCases.xml)

add_executable(extension extension.c)
target_include_directories(extension PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(extension PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME extension_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND extension ${CMAKE_CURRENT_SOURCE_DIR}/extension.xml)

add_executable(newHierachicalRef newHierachicalRef.c)
target_include_directories(newHierachicalRef PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(newHierachicalRef PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME newHierachicalRef_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND newHierachicalRef ${CMAKE_CURRENT_SOURCE_DIR}/newHierachicalRef.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/newHierachicalRef2.xml)

add_executable(references references.c)
target_include_directories(references PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(references PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME references_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND references ${CMAKE_CURRENT_SOURCE_DIR}/references.xml)

add_executable(multipleNamespaces multipleNamespaces.c)
target_include_directories(multipleNamespaces PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(multipleNamespaces PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME multipleNamespaces_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND multipleNamespaces ${CMAKE_CURRENT_SOURCE_DIR}/multipleNamespaces.xml)

add_executable(structMultipleNamespaces structMultipleNamespaces.c)
target_include_directories(structMultipleNamespaces PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(structMultipleNamespaces PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME structMultipleNamespaces_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND structMultipleNamespaces ${CMAKE_CURRENT_SOURCE_DIR}/basestruct.xml ${CMAKE_CURRENT_SOURCE_DIR}/extendedstruct.xml)

add_executable(structwitharray structwitharray.c)
target_include_directories(structwitharray PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(structwitharray PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME structwitharray_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND structwitharray ${CMAKE_CURRENT_SOURCE_DIR}/structwitharray.xml)

add_executable(nodeAttributes nodeAttributes.c)
target_include_directories(nodeAttributes PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(nodeAttributes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME nodeAttributes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND nodeAttributes ${CMAKE_CURRENT_SOURCE_DIR}/nodeAttributes.xml)

add_executable(primitiveValues primitiveValues.c)
target_include_directories(primitiveValues PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(primitiveValues PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME primitiveValues_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND primitiveValues ${CMAKE_CURRENT_SOURCE_DIR}/primitiveValues.xml)

add_executable(issue_246 issue_246.c)
target_include_directories(issue_246 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(issue_246 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME issue_246_Test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND issue_246 ${CMAKE_CURRENT_SOURCE_DIR}/issue_246.xml)

add_executable(issue_246_2 issue_246_2.c)
target_include_directories(issue_246_2 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(issue_246_2 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME issue_246_2_Test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND issue_246_2 ${CMAKE_CURRENT_SOURCE_DIR}/issue_246_2.xml)

add_executable(issue_266 issue_266_testdata.c)
target_include_directories(issue_266 PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(issue_266 PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME issue_266_Test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND issue_266 ${CMAKE_CURRENT_SOURCE_DIR}/issue266_TestData.NodeSet2.xml)

if(${ENABLE_DATATYPEIMPORT_TEST})
    add_subdirectory(dataTypeImport)
endif()

if(${ENABLE_INTEGRATION_TEST})
    add_subdirectory(integration)
endif()
