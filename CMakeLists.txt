cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

include(CTest)
enable_testing()

option(ENABLE_BACKEND_OPEN62541 off "backend for open62541")
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

add_library(xmlImport SHARED src/nodesetLoader.c src/sort.c src/nodeset.c src/charAllocator)
target_include_directories(xmlImport 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIR})
target_link_libraries(xmlImport ${LIBXML2_LIBRARIES})
target_compile_options(xmlImport PRIVATE ${C_COMPILE_DEFS})

#if(${ENABLE_PROFILING})
#    add_executable(profile main.c backend/dummy.c src/nodesetLoader.c src/sort.c src/nodeset.c src/charAllocator.c)
#    target_include_directories(profile
#        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include 
#        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
#    target_compile_options(profile PRIVATE -O0 -g -pg)
#    target_link_libraries(profile CONAN_PKG::libxml2 -pg)
#endif()

add_subdirectory(tests)

#backends
if(${ENABLE_BACKEND_OPEN62541})
    add_subdirectory(open62541)
endif()

add_subdirectory(backends)