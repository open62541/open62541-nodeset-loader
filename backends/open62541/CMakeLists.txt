target_sources(NodesetLoader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/import.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/value.c
)

target_include_directories(NodesetLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
add_subdirectory(examples)