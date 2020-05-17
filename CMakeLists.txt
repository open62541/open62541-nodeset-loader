cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
enable_testing()

option(ENABLE_BACKEND_OPEN62541 off "backend for open62541")
option(ENABLE_ASAN off "build with address sanitizer enabled")
option(ENABLE_INTEGRATION_TEST off "run detailled tests to compare address spaces")
option(ENABLE_DATATYPEIMPORT_TEST off "run tests for importing datatypes")
option(CALC_COVERAGE off "calculate code coverage")

if(CALC_COVERAGE)
    set(COMPILER_OPTIONS -g -O0 -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage)
    set(LIB_GCOV gcov)
endif()

set(C_COMPILE_DEFS -std=c99 -pipe -Wall -Wextra -Wpedantic -Werror 
                    -Wno-static-in-inline # clang doesn't like the use of static inline methods inside static inline methods
                    -Wno-overlength-strings # may happen in the nodeset compiler when complex values are directly encoded
                    -Wno-unused-parameter # some methods may require unused arguments to cast to a method pointer
                    -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
                    -Wformat -Wformat-security -Wformat-nonliteral
                    -Wuninitialized -Winit-self
                    -Wcast-qual
                    -Wstrict-overflow
                    -Wnested-externs
                    -Wmultichar
                    -Wundef
                    -Wc++-compat
                    -Wsign-conversion
                    -Wconversion
                    -Wshadow
                    -fno-strict-aliasing # fewer compiler assumptions about pointer types
                    -fexceptions
                    -Wswitch-enum)


find_package(LibXml2 REQUIRED)
find_package(Check REQUIRED)
find_package(open62541 REQUIRED)

if(${ENABLE_ASAN})
    set(${C_COMPILE_DEFS} -g -fsanitize=address -fsanitize=leak -fsanitize=undefined)
endif()

add_library(NodesetLoader SHARED 
    src/NodesetLoader.c 
    src/Sort.c 
    src/Nodeset.c 
    src/CharAllocator.c 
    src/AliasList.c 
    src/NamespaceList.c
    src/nodes/Node.c
    src/nodes/DataTypeNode.c
    src/nodes/NodeContainer.c
    src/TNodeId.c
    src/PrintfLogger.c
    src/Value.c
    src/InternalRefService.h)
target_include_directories(NodesetLoader
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(NodesetLoader PRIVATE ${LIB_GCOV} ${LIBXML2_LIBRARIES})
target_link_libraries(NodesetLoader PRIVATE open62541::open62541)
target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS} "-fvisibility=hidden")
if(${ENABLE_ASAN})
    target_link_libraries(NodesetLoader INTERFACE "-g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
endif()

add_subdirectory(tests)
add_subdirectory(backends)

#install
set(NODESETLOADER_PUBLIC_HEADER 
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodesetLoader.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/TNodeId.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Logger.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/arch.h
)

if(${ENABLE_BACKEND_OPEN62541})
    list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/backendOpen62541.h)
endif()

set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER "${NODESETLOADER_PUBLIC_HEADER}")

install(TARGETS NodesetLoader
        EXPORT NodesetLoader
        LIBRARY DESTINATION lib/NodesetLoader
        ARCHIVE DESTINATION lib/NodesetLoader
        PUBLIC_HEADER DESTINATION include/NodesetLoader)
install(FILES nodesetloader-config.cmake DESTINATION lib/NodesetLoader)

install(EXPORT NodesetLoader DESTINATION lib/NodesetLoader)