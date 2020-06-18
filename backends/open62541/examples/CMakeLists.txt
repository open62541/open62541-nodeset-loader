add_executable(server server.c)
target_link_libraries(server PRIVATE NodesetLoader open62541)

add_executable(dataTypeImport dataTypeImport.c)
target_link_libraries(dataTypeImport PRIVATE NodesetLoader open62541)

add_executable(iterate iterate.c)
target_link_libraries(iterate PRIVATE NodesetLoader open62541)