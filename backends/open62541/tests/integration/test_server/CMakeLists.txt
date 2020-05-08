add_executable(testServer server.cpp)
target_link_libraries(testServer PRIVATE NodesetLoader open62541::open62541)
