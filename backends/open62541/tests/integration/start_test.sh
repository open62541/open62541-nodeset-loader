#!/bin/bash

echo "START: backend integration test: open62541"
NoOfMandatoryArgs=4
if [[ $# -lt $NoOfMandatoryArgs ]] ; then
    echo "Error: At least " $NoOfMandatoryArgs " script arguments needed: 'path to client' 'client output path' 'path to reference server' 'path to test server'"
    echo "Any additional arguments must be paths to nodesets for test server"
    exit 1
fi

argv=("$@")
argc=$#
CLIENT_BINARY_PATH="${argv[0]}"
TEST_OUTPUT_DIR="${argv[1]}"
REFERENCE_SERVER_BINARY_PATH="${argv[2]}"
TEST_SERVER_BINARY_PATH="${argv[3]}"

mkdir -p "$TEST_OUTPUT_DIR"

echo "Path to client = " $CLIENT_BINARY_PATH
echo "Path to test output = " $TEST_OUTPUT_DIR
echo "Path to reference server = " $REFERENCE_SERVER_BINARY_PATH
echo "Path to test server = " $TEST_SERVER_BINARY_PATH
echo ""

##################################################################
# Reference server:
echo "Start reference server"
"$REFERENCE_SERVER_BINARY_PATH" > "$TEST_OUTPUT_DIR/referenceServerLog.txt" 2>&1 & disown
ReferenceServerPID=$!

echo "Start client: connect to reference server"
CLIENT_OUTPUT_FILE="$TEST_OUTPUT_DIR/clientLog_referenceServer.txt"
"$CLIENT_BINARY_PATH" localhost 4840 "$TEST_OUTPUT_DIR/referenceServer.txt" > "$CLIENT_OUTPUT_FILE" 2>&1
ClientResult=$?

# kill the reference server
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
    if [ $(grep -riq error "$CLIENT_OUTPUT_FILE") ] ; then
        echo "Error: Browsing the addressspace failed"
        exit 1
    fi
fi


##################################################################
# Test server:
echo ""
# get paths to nodesets (additional program arguments)
NodeSetsToLoad=
if [ $argc -gt $NoOfMandatoryArgs ] ; then 
    NodeSetsToLoad=${argv[@]:$NoOfMandatoryArgs}
fi

echo "Start test server with nodesets: " $NodeSetsToLoad
"$TEST_SERVER_BINARY_PATH" $NodeSetsToLoad > "$TEST_OUTPUT_DIR/testServerLog.txt" 2>&1 & disown
TestServerPID=$!

echo "Start client: connect to test server"
CLIENT_OUTPUT_FILE="$TEST_OUTPUT_DIR/clientLog_testServer.txt"
"$CLIENT_BINARY_PATH" localhost 4841 "$TEST_OUTPUT_DIR/testServer.txt" > "$CLIENT_OUTPUT_FILE" 2>&1
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
    if [ $(grep -riq error "$CLIENT_OUTPUT_FILE") ] ; then
        echo "Error: Browsing the addressspace failed"
        exit 1
    fi
fi


##################################################################
echo "Compare results"
diff "$TEST_OUTPUT_DIR/referenceServer.txt" "$TEST_OUTPUT_DIR/testServer.txt" > /dev/null
if [ $? -ne 0 ] ; then
    echo "Error: The addressspaces of the servers do not match"
    exit 1
fi

echo "END: backend integration test: open62541: success"
exit 0