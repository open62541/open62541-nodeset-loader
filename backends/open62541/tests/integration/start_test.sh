#!/bin/bash

echo "START: backend integration test: open62541"

if [[ $# -ne 4 ]] ; then
    echo "Error: 4 script arguments needed: 'path to reference server' | 'path to test server' | 'path to client'"
    exit 1
fi

REFERENCE_SERVER_BINARY_PATH="$1"
TEST_SERVER_BINARY_PATH="$2"
CLIENT_BINARY_PATH="$3"
TEST_OUTPUT_DIR="$4"

mkdir -p "$TEST_OUTPUT_DIR"
CLIENT_OUTPUT_FILE_PATH="$TEST_OUTPUT_DIR"

echo "Path to reference server = " $REFERENCE_SERVER_BINARY_PATH
echo "Path to test server = " $TEST_SERVER_BINARY_PATH
echo "Path to client = " $CLIENT_BINARY_PATH
echo "Path to test output = " $TEST_OUTPUT_DIR

#echo "Start reference server"
#cd $REFERENCE_SERVER_BINARY_PATH
#./referenceServer &

echo "Start client: connect to reference server"
"$CLIENT_BINARY_PATH" localhost 4840 "$TEST_OUTPUT_DIR/referenceServer.txt" > "$CLIENT_OUTPUT_FILE_PATH/referenceServerClientOutput.txt" 2>&1
ClientResult=$?
echo "Result of client:" $ClientResult

if [ $ClientResult -ne 0 ] ; then
    exit 1
else
    # search for error messages within client log
    if [ $(grep -riq error "$CLIENT_OUTPUT_FILE_PATH") ] ; then
        echo "Errors have occurred while browsing the addressspace"
        exit 1
    fi
fi


echo "Start client: connect to test server"
"$CLIENT_BINARY_PATH" localhost 4841 "$TEST_OUTPUT_DIR/testServer.txt" > "$CLIENT_OUTPUT_FILE_PATH/testServerClientOutput.txt" 2>&1
ClientResult=$?
echo "Result of client:" $ClientResult

if [ $ClientResult -ne 0 ] ; then
    exit 1
else
    # search for error messages within client log
    if [ $(grep -riq error "$CLIENT_OUTPUT_FILE_PATH") ] ; then
        echo "Errors have occurred while browsing the addressspace"
        exit 1
    fi
fi

# TODO: compare both files

echo "Everything is awesome"
exit 0