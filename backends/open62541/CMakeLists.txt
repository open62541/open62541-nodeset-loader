target_sources(NodesetLoader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/import.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Value.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DataTypeImporter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/customDataType.c
)

target_include_directories(NodesetLoader PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

add_subdirectory(tests)
add_subdirectory(examples)