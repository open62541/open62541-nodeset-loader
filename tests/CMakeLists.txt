add_executable(parser parser.c $<TARGET_OBJECTS:NodesetLoaderObjects>)
target_include_directories(parser PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(parser PRIVATE open62541::open62541 coverageLib)
add_test(NAME parserTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
         COMMAND parser ${CMAKE_CURRENT_LIST_DIR})
