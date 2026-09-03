cmake_minimum_required(VERSION 3.13)

project(nodesetLoader)

include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(ENABLE_TESTING "enable tests" OFF)
option(ENABLE_EXAMPLES "enable examples" ON)
option(ENABLE_ASAN "build with address sanitizer enabled" OFF)
option(ENABLE_DATATYPEIMPORT_TEST "run tests for importing datatypes" OFF)
option(CALC_COVERAGE "calculate code coverage" OFF)

set(NODESETLOADER_OPEN62541_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/open62541)
if(NOT EXISTS ${NODESETLOADER_OPEN62541_DIR}/CMakeLists.txt OR
   NOT EXISTS ${NODESETLOADER_OPEN62541_DIR}/deps/yxml.c OR
   NOT EXISTS ${NODESETLOADER_OPEN62541_DIR}/deps/yxml.h OR
   NOT EXISTS ${NODESETLOADER_OPEN62541_DIR}/deps/ziptree.c OR
   NOT EXISTS ${NODESETLOADER_OPEN62541_DIR}/deps/ziptree.h)
    message(FATAL_ERROR
        "Standalone builds require the open62541 submodule. "
        "Run: git submodule update --init deps/open62541")
endif()

set(UA_ENABLE_NODESETLOADER OFF CACHE BOOL
    "Do not recursively embed nodesetLoader" FORCE)
add_subdirectory(${NODESETLOADER_OPEN62541_DIR}
                 ${CMAKE_CURRENT_BINARY_DIR}/open62541 EXCLUDE_FROM_ALL)
if(NOT UA_ENABLE_XML_ENCODING)
    message(FATAL_ERROR "NodesetLoader requires UA_ENABLE_XML_ENCODING")
endif()
set(open62541_NODESET_BASE_DIR "${UA_NODESET_DIR}")

add_library(coverageLib INTERFACE)
if(CALC_COVERAGE)
    target_compile_options(coverageLib INTERFACE -O0 -g --coverage)
    target_link_libraries(coverageLib INTERFACE --coverage)
endif()

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX /w44996")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    set(C_COMPILE_DEFS
        -std=c99 -pipe -Wall -Wextra -Wpedantic -Werror
        -Wno-unused-parameter
        -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
        -Wformat -Wformat-security -Wformat-nonliteral
        -Wuninitialized -Winit-self -Wcast-qual -Wstrict-overflow
        -Wnested-externs -Wmultichar -Wundef -Wc++-compat
        -Wsign-conversion -Wconversion -Wshadow -fexceptions)
    set(PTHREAD_LIB pthread)
endif()

if(ENABLE_TESTING)
    find_package(Check REQUIRED)
    if(NOT CHECK_INCLUDE_DIR)
        set(CHECK_INCLUDE_DIR ${CHECK_INCLUDE_DIRS})
    endif()
    if(CHECK_LDFLAGS)
        set(CHECK_LIBRARIES ${CHECK_LDFLAGS})
    endif()
    if(NOT CHECK_LIBRARIES)
        set(CHECK_LIBRARIES Check::check)
    endif()
    include(CTest)
endif()

add_library(NodesetLoader
    src/Nodeset.c
    src/NodesetLoader.c
    backends/open62541/src/import.c
    backends/open62541/src/DataTypeImporter.c)

target_include_directories(NodesetLoader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/src
        ${NODESETLOADER_OPEN62541_DIR}/deps)
target_link_libraries(NodesetLoader PRIVATE open62541::open62541)
target_compile_definitions(NodesetLoader PUBLIC
    $<$<PLATFORM_ID:Linux>:_GNU_SOURCE>)
target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS})
set_target_properties(NodesetLoader PROPERTIES C_VISIBILITY_PRESET hidden)

if(CALC_COVERAGE)
    target_link_libraries(NodesetLoader PUBLIC coverageLib)
endif()
if(ENABLE_ASAN)
    target_link_libraries(NodesetLoader INTERFACE
        "-g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
endif()

if(ENABLE_EXAMPLES)
    add_subdirectory(backends/open62541/examples)
endif()
if(ENABLE_TESTING)
    add_subdirectory(backends/open62541/tests)
    add_subdirectory(tests)
endif()
if(CALC_COVERAGE)
    add_subdirectory(coverage)
endif()

if(NOT CALC_COVERAGE)
    set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER
        "${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodesetLoader.h")
    install(TARGETS NodesetLoader
            EXPORT NodesetLoader
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION include/NodesetLoader)
    install(FILES nodesetloader-config.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)
    install(EXPORT NodesetLoader
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)
endif()
