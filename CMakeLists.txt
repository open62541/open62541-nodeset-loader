cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(ENABLE_CONAN "use conan for consuming dependencies" on)
option(ENABLE_TESTING "enable tests" off)
option(ENABLE_EXAMPLES "enable examples" on)
option(ENABLE_BACKEND_OPEN62541 "backend for open62541" off)
option(ENABLE_BACKEND_STDOUT "backend for stdout" on)
option(ENABLE_ASAN "build with address sanitizer enabled" off)
option(ENABLE_INTEGRATION_TEST "run detailled tests to compare address spaces" off)
option(ENABLE_BUILD_INTO_OPEN62541 "make nodesetLoader part of the open62541 library" off)
option(ENABLE_DATATYPEIMPORT_TEST "run tests for importing datatypes" off)
option(CALC_COVERAGE "calculate code coverage" off)

# TODO: Include integration tests after support for XML Data
#       Encoding has been added to the open62541 >= 1.3.2.
#       Currently, the integration tests will fail for
#       open62541 v1.3.2 and disable successful CI build.
set(ENABLE_INTEGRATION_TEST off)

if(${ENABLE_BUILD_INTO_OPEN62541})
    # Disable conan, since open62541 provider all integration dependencies
    set(ENABLE_CONAN off)

    # Disble code coverage, since this will be covered within open62541
    set(CALC_COVERAGE off)

    # TODO: Enable nodesetLoader tests within open62541 test suite
    set(ENABLE_TESTING off)

    # TODO: Enable nodesetLoader examples within open62541
    set(ENABLE_EXAMPLES off)

    set(ENABLE_BACKEND_OPEN62541 on)
    set(ENABLE_BACKEND_STDOUT off)
endif()

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

    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

if(NOT ${ENABLE_BUILD_INTO_OPEN62541})
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
                            -Wno-unused-parameter
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
                            -fexceptions
                            -Wswitch-enum)

        set(PTHREAD_LIB pthread)
    endif()
endif()

if(${ENABLE_BUILD_INTO_OPEN62541})
    string(REPLACE "-" ";" MAIN_VERSION_OPEN62541_LIST ${OPEN62541_VERSION})
    list(GET MAIN_VERSION_OPEN62541_LIST 0 MAIN_VERSION_OPEN62541)
    if(NOT ${MAIN_VERSION_OPEN62541} MATCHES "v1.3.2")
        message(FATAL_ERROR "This version of nodesetLoader can be built with open62541 v1.3.2")
    endif()
elseif(${ENABLE_BACKEND_OPEN62541})
    find_package(open62541 REQUIRED)
    if(open62541_VERSION VERSION_LESS "1.3.x")
        set(USE_MEMBERTYPE_INDEX on)
        set(open62541_NODESET_BASE_DIR "${open62541_NODESET_DIR}")
    else()
        set(USE_MEMBERTYPE_INDEX off)
        set(open62541_NODESET_BASE_DIR "${UA_NODESET_DIR}")
    endif()
endif()
find_package(LibXml2 REQUIRED)

if(${ENABLE_TESTING})
    find_package(Check REQUIRED)
    include(CTest)
endif()

add_subdirectory(backends)

set(NODESETLOADER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NodesetLoader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sort.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Nodeset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CharAllocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AliasList.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NamespaceList.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/Node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/DataTypeNode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/NodeContainer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/InstanceNode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NodeId.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PrintfLogger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Value.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InternalRefService.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Parser.c
    ${NODESETLOADER_BACKEND_SOURCES}
    CACHE INTERNAL "")

set(NODESETLOADER_PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${NODESETLOADER_BACKEND_PUBLIC_INCLUDES}
    CACHE INTERNAL "")

set(NODESETLOADER_PRIVATE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${NODESETLOADER_BACKEND_PRIVATE_INCLUDES}
    ${LIBXML2_INCLUDE_DIRS}
    CACHE INTERNAL "")

set(NODESETLOADER_DEPS_LIBS
    ${LIBXML2_LIBRARIES}
    ${NODESETLOADER_BACKEND_DEPS_LIBS}
    CACHE INTERNAL "")

set(NODESETLOADER_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodesetLoader.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodeId.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Logger.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/arch.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/ReferenceService.h
    ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Extension.h
    ${NODESETLOADER_BACKEND_PUBLIC_HEADERS}
    CACHE INTERNAL "")

if(NOT ${ENABLE_BUILD_INTO_OPEN62541})
    add_library(NodesetLoader ${NODESETLOADER_SOURCES})

    target_include_directories(NodesetLoader
        PUBLIC  ${NODESETLOADER_PUBLIC_INCLUDES}
        PRIVATE ${NODESETLOADER_PRIVATE_INCLUDES})

    target_link_libraries(NodesetLoader PRIVATE ${NODESETLOADER_DEPS_LIBS})
    if(${CALC_COVERAGE})
        target_link_libraries(NodesetLoader PUBLIC coverageLib)
    endif()

    if(${ENABLE_BACKEND_OPEN62541})
        target_include_directories(NodesetLoader PRIVATE open62541::open62541)
        target_link_libraries(NodesetLoader PRIVATE open62541::open62541)
        if(${USE_MEMBERTYPE_INDEX})
            target_compile_definitions(NodesetLoader PUBLIC -DUSE_MEMBERTYPE_INDEX=1)
        endif()
    endif()

    # TODO: Speficy cleanup of custom data types for a specific open62541 version
    target_compile_definitions(NodesetLoader PUBLIC -DUSE_CLEANUP_CUSTOM_DATATYPES=1)

    target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS})
    set_target_properties(NodesetLoader PROPERTIES C_VISIBILITY_PRESET hidden)
    if(${ENABLE_ASAN})
        target_link_libraries(NodesetLoader INTERFACE "-g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
    endif()
endif()

if(${ENABLE_TESTING})
    add_subdirectory(tests)
endif()

if(${CALC_COVERAGE})
    add_subdirectory(coverage)
endif()

#install
if(NOT ${CALC_COVERAGE} AND NOT ${ENABLE_BUILD_INTO_OPEN62541})
    set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER "${NODESETLOADER_PUBLIC_HEADERS}")

    install(TARGETS NodesetLoader
            EXPORT NodesetLoader
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION include/NodesetLoader)

    install(FILES nodesetloader-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)

    install(EXPORT NodesetLoader DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)
endif()
