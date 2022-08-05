target_sources(NodesetLoader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/import.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Value.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DataTypeImporter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/customDataType.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/RefServiceImpl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ServerContext.c
)

target_include_directories(NodesetLoader PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

if(ENABLE_TESTING)
    add_subdirectory(tests)
endif()
if(${ENABLE_EXAMPLES})
    add_subdirectory(examples)
endif()