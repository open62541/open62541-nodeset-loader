target_sources(xmlImport
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/import.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/value.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/conversion.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/value.h
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openBackend.h  # poor PUBLIC example, see discussion below for why
)

target_link_libraries(xmlImport PRIVATE open62541)

#add_subdirectory(tests)
add_subdirectory(examples)