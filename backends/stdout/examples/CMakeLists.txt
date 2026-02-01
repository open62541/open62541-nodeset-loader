add_executable(parserDemo main.c)
target_link_libraries(parserDemo PRIVATE NodesetLoader)
target_link_libraries(parserDemo PRIVATE open62541::open62541)
