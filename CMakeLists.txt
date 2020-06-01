cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(CONANFILE conanfile.txt
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

include(CTest)
enable_testing()

option(ENABLE_BACKEND_OPEN62541 off "backend for open62541")
option(ENABLE_ASAN off "build with address sanitizer enabled")
option(ENABLE_INTEGRATION_TEST off "run detailled tests to compare address spaces")
option(ENABLE_DATATYPEIMPORT_TEST off "run tests for importing datatypes")
option(CALC_COVERAGE off "calculate code coverage")

if(CALC_COVERAGE)
    set(GCOV_COMPILE_OPTIONS -g -O0 -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage)
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
    src/InternalRefService.c
    src/Parser.c)

target_include_directories(NodesetLoader
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(NodesetLoader PRIVATE ${LIB_GCOV} ${LIBXML2_LIBRARIES})
target_link_libraries(NodesetLoader PRIVATE open62541::open62541)
target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS} "-fvisibility=hidden")
target_compile_options(NodesetLoader PUBLIC ${GCOV_COMPILE_DEFS})
target_link_libraries(NodesetLoader PRIVATE gcov)
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
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/ReferenceService.h
)

if(${ENABLE_BACKEND_OPEN62541})
    list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/backendOpen62541.h)
endif()

set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER "${NODESETLOADER_PUBLIC_HEADER}")

install(TARGETS NodesetLoader
        EXPORT NodesetLoader
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/NodesetLoader)
install(FILES nodesetloader-config.cmake DESTINATION lib)

install(EXPORT NodesetLoader DESTINATION lib)