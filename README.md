# nodesetLoader [![Build Status](https://travis-ci.org/matkonnerth/nodesetLoader.svg?branch=master)](https://travis-ci.org/matkonnerth/nodesetLoader)
nodesetLoader is a library written in C99 for the purpose of loading OPC UA nodesets from xml and sorting the nodes based on their hierachical references.

# Current status
NodesetLoader is in an early alpha, SHOULD NOT be used in production environment. Interfaces for consumers will change in future.
Automated test environment for quality insurance is set up, but has to be improved.

## License
MPL2.0 https://github.com/matkonnerth/nodesetLoader/blob/master/LICENSE

## Current status

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


