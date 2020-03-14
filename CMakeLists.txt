cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

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

add_library(xmlImport SHARED src/nodesetLoader.c src/sort.c src/nodeset.c src/charAllocator)
target_include_directories(xmlImport 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(xmlImport CONAN_PKG::libxml2)
target_compile_options(xmlImport PRIVATE ${C_COMPILE_DEFS})


add_executable(parserDemo main.c backend/dummy.c)
target_link_libraries(parserDemo xmlImport)
target_compile_options(parserDemo PRIVATE ${C_COMPILE_DEFS})

#target for profiling
add_executable(profile main.c backend/dummy.c src/nodesetLoader.c src/sort.c src/nodeset.c src/charAllocator.c)
target_include_directories(profile
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(profile PRIVATE -O0 -g -pg)
target_link_libraries(profile CONAN_PKG::libxml2 -pg)

add_subdirectory(tests)

