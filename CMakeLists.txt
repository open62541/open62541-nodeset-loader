cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(ENABLE_CONAN "use conan for consuming dependencies" on)
option(ENABLE_TESTING "enable tests" off)
option(ENABLE_BACKEND_OPEN62541 "backend for open62541" off)
option(ENABLE_ASAN "build with address sanitizer enabled" off)
option(ENABLE_INTEGRATION_TEST "run detailled tests to compare address spaces" off)
option(ENABLE_DATATYPEIMPORT_TEST "run tests for importing datatypes" off)
option(CALC_COVERAGE "calculate code coverage" off)


if(${ENABLE_CONAN})
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)
    conan_cmake_run(CONANFILE conanfile.txt
                    BASIC_SETUP
                    CMAKE_TARGETS
                    BUILD missing)
endif()

#link against this library to gather coverage info
add_library(coverageLib INTERFACE)

if(${CALC_COVERAGE})
    #set(GCOV_COMPILE_OPTIONS -g -O0 -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage)

    message(STATUS "Code coverage is enabled.")
    # Note that --coverage is synonym for the necessary compiler and
    # linker flags for the given compiler.  For example, with GCC,
    # --coverage translates to -fprofile-arcs -ftest-coverage when
    # compiling and -lgcov when linking
    #add_compile_options(--coverage -O0)
    #add_link_options(--coverage)
    target_compile_options(coverageLib INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags)
    )
    target_link_libraries(coverageLib INTERFACE --coverage)
endif()

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX /w44996") # Compiler warnings, error on warning
endif()

if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
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

    set(PTHREAD_LIB pthread)
endif()

find_package(open62541 REQUIRED)
find_package(LibXml2 REQUIRED)

add_library(NodesetLoader
    src/NodesetLoader.c 
    src/Sort.c 
    src/Nodeset.c 
    src/CharAllocator.c 
    src/AliasList.c 
    src/NamespaceList.c
    src/nodes/Node.c
    src/nodes/DataTypeNode.c
    src/nodes/NodeContainer.c
    src/nodes/InstanceNode.c
    src/TNodeId.c
    src/PrintfLogger.c
    src/Value.c
    src/InternalRefService.c
    src/Parser.c)

target_include_directories(NodesetLoader
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(NodesetLoader PRIVATE ${LIBXML2_LIBRARIES})
if(${CALC_COVERAGE})
    target_link_libraries(NodesetLoader PUBLIC coverageLib)
endif()
target_include_directories(NodesetLoader PRIVATE open62541)
target_link_libraries(NodesetLoader PRIVATE open62541)
target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS})
set_target_properties(NodesetLoader PROPERTIES C_VISIBILITY_PRESET hidden)
if(${ENABLE_ASAN})
    target_link_libraries(NodesetLoader INTERFACE "-g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
endif()

if(ENABLE_TESTING)
    if(MSVC)
        set(CHECK_INSTALL_DIR "C:/Program Files (x86)/Check")
    endif()
    find_package(Check REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
add_subdirectory(backends)

if(${CALC_COVERAGE})
    add_subdirectory(coverage)
endif()

#install
if(NOT ${CALC_COVERAGE})
    set(NODESETLOADER_PUBLIC_HEADER 
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodesetLoader.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/TNodeId.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Logger.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/arch.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/ReferenceService.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Extension.h
    )

    if(${ENABLE_BACKEND_OPEN62541})
        list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/backendOpen62541.h)
        list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/dataTypes.h)
    endif()

    set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER "${NODESETLOADER_PUBLIC_HEADER}")

    install(TARGETS NodesetLoader
            EXPORT NodesetLoader
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
            PUBLIC_HEADER DESTINATION include/NodesetLoader)
    install(FILES nodesetloader-config.cmake DESTINATION lib/cmake/NodesetLoader)

    install(EXPORT NodesetLoader DESTINATION lib/cmake/NodesetLoader)
endif()
