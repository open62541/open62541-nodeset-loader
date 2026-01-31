add_executable(enum enum.c)
target_include_directories(enum PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(enum PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME enum_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND enum ${CMAKE_CURRENT_SOURCE_DIR}/enum.xml)

add_executable(subDataType subDataType.c)
target_include_directories(subDataType PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(subDataType PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME subDataType_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND subDataType ${CMAKE_CURRENT_SOURCE_DIR}/subDataType.xml)

ua_generate_nodeset_and_datatypes(
    NAME "struct"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/struct.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/struct.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml"
    INTERNAL
)

add_executable(compareStructTypes compareStructTypes.c ${UA_TYPES_STRUCT_SOURCES})
add_dependencies(compareStructTypes open62541-generator-ns-struct)
target_link_libraries(compareStructTypes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareStructTypes PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareStructTypes PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareStructTypes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructTypes "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml")

ua_generate_nodeset_and_datatypes(
    NAME "structExtended"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.xml"
    INTERNAL
)

add_executable(compareStructExtended compareStructExtended.c ${UA_TYPES_STRUCTEXTENDED_SOURCES})
add_dependencies(compareStructExtended open62541-generator-ns-structExtended)
target_link_libraries(compareStructExtended PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareStructExtended PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareStructExtended PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareStructExtended_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructExtended "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.xml")

ua_generate_nodeset_and_datatypes(
    NAME "specializedStruct"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.xml"
    DEPENDS "struct"
    INTERNAL
)

add_executable(compareStructSpecialized compareStructSpecialized.c ${UA_TYPES_SPECIALIZEDSTRUCT_SOURCES} ${UA_TYPES_STRUCT_SOURCES})
add_dependencies(compareStructSpecialized open62541-generator-ns-specializedStruct)
target_link_libraries(compareStructSpecialized PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareStructSpecialized PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareStructSpecialized PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareStructSpecialized_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructSpecialized "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml" "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.xml")


ua_generate_nodeset_and_datatypes(
    NAME "optionalstruct"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/optionalstruct.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/optionalstruct.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/optionalstruct.xml"
    INTERNAL
)

add_executable(compareOptionalStruct optionalStruct.c ${UA_TYPES_OPTIONALSTRUCT_SOURCES})
add_dependencies(compareOptionalStruct open62541-generator-ns-optionalstruct)
target_link_libraries(compareOptionalStruct PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareOptionalStruct PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareOptionalStruct PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareOptionalStruct_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareOptionalStruct "${CMAKE_CURRENT_SOURCE_DIR}/optionalstruct.xml")

ua_generate_nodeset_and_datatypes(
    NAME "union"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/union.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/union.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/union.xml"
    INTERNAL
)

add_executable(compareUnion compareUnion.c ${UA_TYPES_UNION_SOURCES})
add_dependencies(compareUnion open62541-generator-ns-union)
target_link_libraries(compareUnion PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareUnion PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareUnion PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareUnion_Test
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
  COMMAND compareUnion "${CMAKE_CURRENT_SOURCE_DIR}/union.xml")

ua_generate_nodeset_and_datatypes(
    NAME "abstractdatatypemember"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/abstractdatatypemember.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/abstractdatatypemember.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/abstractdatatypemember.xml"
    INTERNAL
)

add_executable(compareAbstractDataTypeMember compareAbstractDataTypeMember.c ${UA_TYPES_ABSTRACTDATATYPEMEMBER_SOURCES})
add_dependencies(compareAbstractDataTypeMember open62541-generator-ns-abstractdatatypemember)
target_link_libraries(compareAbstractDataTypeMember PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareAbstractDataTypeMember PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareAbstractDataTypeMember PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareAbstractDataTypeMember_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareAbstractDataTypeMember "${CMAKE_CURRENT_SOURCE_DIR}/abstractdatatypemember.xml")



ua_generate_nodeset_and_datatypes(
    NAME "optionsetgen"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/optionset.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/optionset.bsd"
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/optionset.xml"
    INTERNAL
)

add_executable(compareOptionset compareOptionset.c ${UA_TYPES_OPTIONSETGEN_SOURCES})
add_dependencies(compareOptionset open62541-generator-ns-optionsetgen)
target_link_libraries(compareOptionset PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
target_include_directories(compareOptionset PRIVATE ${CMAKE_BINARY_DIR}/src_generated)
target_include_directories(compareOptionset PRIVATE ${CHECK_INCLUDE_DIR})

add_test(NAME compareOptionset_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareOptionset "${CMAKE_CURRENT_SOURCE_DIR}/optionset.xml")

add_executable(bytestring bytestring.c)
target_include_directories(bytestring PRIVATE ${CHECK_INCLUDE_DIR})
target_link_libraries(bytestring PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES} ${PTHREAD_LIB})
add_test(NAME bytestring_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bytestring ${CMAKE_CURRENT_SOURCE_DIR}/bytestring.xml)
