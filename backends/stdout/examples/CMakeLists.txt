add_executable(parserDemo main.c dump.c)
target_link_libraries(parserDemo PRIVATE NodesetLoader)