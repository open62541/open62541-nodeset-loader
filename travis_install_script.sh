#!/bin/bash
set -e

# gcc, test, memcheck
if ! [ -z ${GCC_MEMCHECK+x} ]; then
    echo ${PWD}
    git clone https://github.com/open62541/open62541.git
    cd open62541
    git submodule init && git submodule update
    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL ..
    make -j
    sudo make install
fi

# clang, test
if ! [ -z ${CLANG_RELEASE+x} ]; then
    echo ${PWD}
    git clone https://github.com/open62541/open62541.git
    cd open62541
    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=REDUCED ..
    make -j
    sudo make install
fi

# integrationTest, here we need the full namespace zero
if ! [ -z ${INTEGRATION_TEST+x} ]; then
    echo ${PWD}
    git clone https://github.com/open62541/open62541.git
    cd open62541
    git submodule init && git submodule update
    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL ..
    make -j
    sudo make install
fi
