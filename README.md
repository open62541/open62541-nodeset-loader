# nodesetLoader [![Build Status](https://travis-ci.org/matkonnerth/nodesetLoader.svg?branch=master)](https://travis-ci.org/matkonnerth/nodesetLoader)
nodesetLoader is a library written in C99 for the purpose of loading OPC UA nodesets from xml and sorting the nodes based on their hierachical references.

## License
MPL2.0 https://github.com/matkonnerth/nodesetLoader/blob/master/LICENSE

# Current status
First official release v0.1.0 is tagged. Please be aware that interface may change in future releases.

#### Backend open62541
Support for loading values with datatypes from namespace 0 or custom namespacesd
Support parsing of extensions (via a callback interface)

## dependencies
xmlImport: libXml (http://www.xmlsoft.org/) for parsing the nodeset xml \
unit testing: libcheck

## Design goals
1) performance
2) memory overhead

## Build
mkdir build \
cd build \
cmake .. \
make

## Running the demo
./parserDemo pathToNodesetFile1 pathToNodesetFile2
  
## Integration with open62541

### example

```c
#include <NodesetLoader/backendOpen62541.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
  running = false;
}

int main(int argc, const char *argv[]) {
  UA_Server *server = UA_Server_new();
  UA_ServerConfig_setDefault(UA_Server_getConfig(server));
  //provide the server and the path to nodeset
  //returns true in case of successful import
  if(!NodesetLoader_loadFile(server, "../Opc.Ua.Di.NodeSet2.xml", NULL))
  {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "importing the xml nodeset failed");
  }
  UA_StatusCode retval = UA_Server_run(server, &running);
  //NodesetLoader is allocating memory for custom dataTypes, user has to manually clean up
  cleanupCustomTypes(UA_Server_getConfig(server)->customDataTypes);
  UA_Server_delete(server);
  return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

### status
* :heavy_check_mark: import of multiple nodeset files
* :heavy_check_mark: nodesetLoader uses the logger from the server configuration
* :heavy_check_mark: DataType import: custom datatypes
* :heavy_check_mark: DataType import: optionset, union, structs with optional members supported
* :heavy_check_mark: Value import: for variables with datatypes from namespace 0 and custom data types

### build

build with cmake option ENABLE_BACKEND_OPEN62541

There is an example in the open backend, can be started with
backends/open62541/examples/server <pathToNodeset>

Here's an example repo, consuming open62541 and NodesetLoader via cmake find_package:
https://github.com/matkonnerth/nodesetLoader_usage


### conan package

conan packages are available on bintray repo https://bintray.com/matkonnerth/cpprepo
repo containing the conan recipe: https://github.com/matkonnerth/conan-nodesetloader

