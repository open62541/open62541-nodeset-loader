cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
enable_testing()

option(ENABLE_BACKEND_OPEN62541 off "backend for open62541")
option(ENABLE_ASAN off "build with address sanitizer enabled")
#option(ENABLE_PROFILING off "enable profiling")

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
                    -fexceptions) # recommended for multi-threaded C code, also in combination with C++ code

find_package(LibXml2 REQUIRED)
find_package(Check REQUIRED)

if(${ENABLE_ASAN})
    set(${C_COMPILE_DEFS} -g -fsanitize=address -fsanitize=leak -fsanitize=undefined)
endif()

#backends
if(${ENABLE_BACKEND_OPEN62541})
    add_subdirectory(open62541)
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
    src/nodes/NodeContainer.c)
target_include_directories(NodesetLoader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIR})
target_link_libraries(NodesetLoader PRIVATE ${LIBXML2_LIBRARIES})
if(${ENABLE_BACKEND_OPEN62541})
    target_link_libraries(NodesetLoader PRIVATE open62541)
endif()
target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS})

#if(${ENABLE_PROFILING})
#    add_executable(profile main.c backend/dummy.c src/nodesetLoader.c src/sort.c src/nodeset.c src/charAllocator.c)
#    target_include_directories(profile
#        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include 
#        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
#    target_compile_options(profile PRIVATE -O0 -g -pg)
#    target_link_libraries(profile CONAN_PKG::libxml2 -pg)
#endif()

add_subdirectory(tests)
add_subdirectory(backends)