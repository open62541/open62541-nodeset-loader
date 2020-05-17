add_executable(enum enum.c)
target_link_libraries(enum PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME enum_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND enum ${CMAKE_CURRENT_SOURCE_DIR}/enum.xml)

add_executable(subDataType subDataType.c)
target_link_libraries(subDataType PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME subDataType_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND subDataType ${CMAKE_CURRENT_SOURCE_DIR}/subDataType.xml)

ua_generate_nodeset_and_datatypes(
    NAME "di"
    FILE_CSV "${open62541_NODESET_DIR}/DI/OpcUaDiModel.csv"
    FILE_BSD "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
    NAMESPACE_IDX 2
    FILE_NS "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    INTERNAL
)

add_executable(compareDITypes compareDITypes.c ${UA_TYPES_DI_SOURCES})
add_dependencies(compareDITypes open62541-generator-ns-di)
target_link_libraries(compareDITypes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
target_include_directories(compareDITypes PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

add_test(NAME compareDITypes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareDITypes "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")

ua_generate_nodeset_and_datatypes(
    NAME "struct"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/struct.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/struct.bsd"
    NAMESPACE_IDX 2
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml"
    INTERNAL
)

add_executable(compareStructTypes compareStructTypes.c ${UA_TYPES_STRUCT_SOURCES})
add_dependencies(compareStructTypes open62541-generator-ns-struct)
target_link_libraries(compareStructTypes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
target_include_directories(compareStructTypes PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

add_test(NAME compareStructTypes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructTypes "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml")

ua_generate_nodeset_and_datatypes(
    NAME "structExtended"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.bsd"
    NAMESPACE_IDX 2
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.xml"
    INTERNAL
)

add_executable(compareStructExtended compareStructExtended.c ${UA_TYPES_STRUCTEXTENDED_SOURCES})
add_dependencies(compareStructExtended open62541-generator-ns-structExtended)
target_link_libraries(compareStructExtended PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
target_include_directories(compareStructExtended PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

add_test(NAME compareStructExtended_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructExtended "${CMAKE_CURRENT_SOURCE_DIR}/structExtended.xml")

ua_generate_nodeset_and_datatypes(
    NAME "specializedStruct"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.csv"
    FILE_BSD "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.bsd"
    NAMESPACE_IDX 3
    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.xml"
    DEPENDS "struct"
    INTERNAL
)

add_executable(compareStructSpecialized compareStructSpecialized.c ${UA_TYPES_SPECIALIZEDSTRUCT_SOURCES} ${UA_TYPES_STRUCT_SOURCES})
add_dependencies(compareStructSpecialized open62541-generator-ns-specializedStruct)
target_link_libraries(compareStructSpecialized PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
target_include_directories(compareStructSpecialized PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

add_test(NAME compareStructSpecialized_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareStructSpecialized "${CMAKE_CURRENT_SOURCE_DIR}/struct.xml" "${CMAKE_CURRENT_SOURCE_DIR}/specializedstruct.xml")