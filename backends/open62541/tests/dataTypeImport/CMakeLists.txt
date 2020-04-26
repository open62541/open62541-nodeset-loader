add_executable(struct struct.c)
target_link_libraries(struct PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME struct_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND struct ${CMAKE_CURRENT_SOURCE_DIR}/struct.xml)

add_executable(enum enum.c)
target_link_libraries(enum PRIVATE NodesetLoader open62541 ${CHECK_LIBRARIES})
add_test(NAME enum_Test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
    COMMAND enum ${CMAKE_CURRENT_SOURCE_DIR}/enum.xml)