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

# gcc, test
if ! [ -z ${GCC_RELEASE+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON .. 
    make -j
    make test
fi

# gcc + asan, test
if ! [ -z ${GCC_ASAN+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_ASAN=ON .. 
    make -j
    make test
fi

# clang, test
if ! [ -z ${CLANG_RELEASE+x} ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON  ..
    make
    make test
fi

# coverage
if ! [ -z ${COVERAGE+x} ]; then
    mkdir -p build
    cd build
    conan install -s compiler.libcxx=libstdc++11 ..
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCALC_COVERAGE=ON .. 
    make -j
    make test
fi


