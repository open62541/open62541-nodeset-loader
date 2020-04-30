add_executable(struct struct.c)
target_link_libraries(struct PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME struct_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND struct ${CMAKE_CURRENT_SOURCE_DIR}/struct.xml)

add_executable(enum enum.c)
target_link_libraries(enum PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
add_test(NAME enum_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND enum ${CMAKE_CURRENT_SOURCE_DIR}/enum.xml)

ua_generate_nodeset_and_datatypes(
    NAME "di"
    FILE_CSV "${open62541_NODESET_DIR}/DI/OpcUaDiModel.csv"
    FILE_BSD "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
    NAMESPACE_IDX 2
    FILE_NS "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    INTERNAL
)

message(warning ${UA_TYPES_DI_SOURCES})
add_executable(compareDITypes compareDITypes.c ${UA_NODESET_DI_SOURCES} ${UA_TYPES_DI_SOURCES})
add_dependencies(compareDITypes open62541-generator-ns-di)
target_link_libraries(compareDITypes PRIVATE NodesetLoader open62541::open62541 ${CHECK_LIBRARIES})
target_include_directories(compareDITypes PRIVATE ${CMAKE_BINARY_DIR}/src_generated)

add_test(NAME compareDITypes_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND compareDITypes "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")
