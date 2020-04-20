
add_library(backendOpen62541 STATIC src/import.c src/value.c)
target_include_directories(backendOpen62541 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(backendOpen62541 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(backendOpen62541 PRIVATE xmlImport)
target_link_libraries(backendOpen62541 PUBLIC open62541)

#add_subdirectory(tests)
add_subdirectory(examples)