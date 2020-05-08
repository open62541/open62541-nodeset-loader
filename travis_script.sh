#!/bin/bash
set -e

# gcc, test, memcheck
if ! [ -z ${GCC_MEMCHECK+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DENABLE_BACKEND_OPEN62541=ON .. 
    make -j
    make test
    ctest --overwrite MemoryCheckCommandOptions="--leak-check=full --error-exitcode=100" -T memcheck
fi

# clang, test
if ! [ -z ${CLANG_RELEASE+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebug -DBUILD_SHARED_LIBS=ON -DENABLE_BACKEND_OPEN62541=ON ..
    make
    make test
fi

# integrationTest
if ! [ -z ${INTEGRATION_TEST+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebug -DBUILD_SHARED_LIBS=ON -DENABLE_BACKEND_OPEN62541=ON -DENABLE_INTEGRATION_TEST ..
    make
    make test
fi
