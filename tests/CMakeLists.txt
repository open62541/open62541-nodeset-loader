add_executable(parser parser.c)
target_include_directories(parser PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(parser PRIVATE NodesetLoader open62541::open62541)
add_test(NAME parserTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
         COMMAND parser ${CMAKE_CURRENT_LIST_DIR})
