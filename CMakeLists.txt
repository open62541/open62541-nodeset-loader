cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_definitions(-std=c99 -pipe -Wall -Wextra -Wpedantic -Wno-unused-parameter)
project(xmlImport)

find_package(LibXml2 REQUIRED)

add_library(xmlImport SHARED src/nodesetLoader.c src/util.c src/sort.c src/nodeset.c)
target_include_directories(xmlImport 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIR})
target_link_libraries(xmlImport ${LIBXML2_LIBRARIES} rt)


add_executable(parserDemo main.c backend/dummy.c)
target_link_libraries(parserDemo xmlImport)

#target for profiling
add_executable(profile main.c backend/dummy.c src/nodesetLoader.c src/util.c src/sort.c src/nodeset.c)
target_include_directories(profile
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIR})
target_compile_options(profile PRIVATE -O0 -g -pg)
target_link_libraries(profile ${LIBXML2_LIBRARIES} -pg)

add_subdirectory(tests)