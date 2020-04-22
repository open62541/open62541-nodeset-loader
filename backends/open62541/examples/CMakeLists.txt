add_executable(server server.c)
target_link_libraries(server PRIVATE xmlImport open62541)