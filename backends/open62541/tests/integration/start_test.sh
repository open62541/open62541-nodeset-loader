#!/bin/bash

echo "START: backend integration test: open62541"

if [[ $# -ne 6 ]] ; then
    echo "Error: 6 script arguments needed: 'path to reference server' | 'path to test server' | 'path to open62541 nodesets' | 'path to local test nodesets' | path to client'"
    exit 1
fi

REFERENCE_SERVER_BINARY_PATH="$1"
TEST_SERVER_BINARY_PATH="$2"
NODESET_PATH_OPEN62541="$3"
NODESET_PATH_BACKEND_TESTS="$4"
CLIENT_BINARY_PATH="$5"
TEST_OUTPUT_DIR="$6"

mkdir -p "$TEST_OUTPUT_DIR"
CLIENT_OUTPUT_FILE_PATH="$TEST_OUTPUT_DIR"

echo "Path to reference server = " $REFERENCE_SERVER_BINARY_PATH
echo "Path to test server = " $TEST_SERVER_BINARY_PATH
echo "Nodeset path open62541 = " $NODESET_PATH_OPEN62541
echo "Nodeset path backend tests = " $NODESET_PATH_BACKEND_TESTS
echo "Path to client = " $CLIENT_BINARY_PATH
echo "Path to test output = " $TEST_OUTPUT_DIR
echo ""

echo "Start reference server"
"$REFERENCE_SERVER_BINARY_PATH" > "$TEST_OUTPUT_DIR/referenceServerLog.txt" 2>&1 & disown
ReferenceServerPID=$!

echo "Start client: connect to reference server"
"$CLIENT_BINARY_PATH" localhost 4840 "$TEST_OUTPUT_DIR/referenceServer.txt" > "$CLIENT_OUTPUT_FILE_PATH/clientLog_referenceServer.txt" 2>&1
ClientResult=$?

echo "Kill reference server"
kill -9 $ReferenceServerPID
if [ $? -ne 0 ] ; then
    echo "Error: killing the reference server failed"
    echo "PID = " $ReferenceServerPID
    exit 1
fi

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

echo ""
echo "Start test server"
"$TEST_SERVER_BINARY_PATH" \
    "$NODESET_PATH_OPEN62541/DI/Opc.Ua.Di.NodeSet2.xml" \
    "$NODESET_PATH_OPEN62541/PLCopen/Opc.Ua.Plc.NodeSet2.xml" \
        > "$TEST_OUTPUT_DIR/testServerLog.txt" 2>&1 & disown
TestServerPID=$!

#    "$NODESET_PATH_BACKEND_TESTS/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml" \
#    "$NODESET_PATH_BACKEND_TESTS/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml" \

echo "Start client: connect to test server"
"$CLIENT_BINARY_PATH" localhost 4841 "$TEST_OUTPUT_DIR/testServer.txt" > "$CLIENT_OUTPUT_FILE_PATH/clientLog_testServer.txt" 2>&1
ClientResult=$?
echo "Result of client:" $ClientResult

echo "Kill test server"
kill -9 $TestServerPID
if [ $? -ne 0 ] ; then
    echo "Error: killing the test server failed"
    echo "PID = " $TestServerPID
    exit 1
fi

if [ $ClientResult -ne 0 ] ; then
    exit 1
else
    # search for error messages within client log
    if [ $(grep -riq error "$CLIENT_OUTPUT_FILE_PATH") ] ; then
        echo "Errors have occurred while browsing the addressspace"
        exit 1
    fi
fi

echo "Compare results"
diff "$TEST_OUTPUT_DIR/referenceServer.txt" "$TEST_OUTPUT_DIR/testServer.txt" > /dev/null
if [ $? -ne 0 ] ; then
    echo "Error: The address spaces of the servers do not match"
    exit 1
fi

echo "Successful test!"
exit 0