# nodesetLoader [![Build Status](https://travis-ci.org/matkonnerth/nodesetLoader.svg?branch=master)](https://travis-ci.org/matkonnerth/nodesetLoader)
nodesetLoader is a library written in C99 for the purpose of loading OPC UA nodesets from xml and sorting the nodes based on their hierachical references.

## License
MPL2.0 https://github.com/matkonnerth/nodesetLoader/blob/master/LICENSE

## Current status
Support for loading values with DataType from namespace 0 was added. Showcase is https://github.com/matkonnerth/openWrapper
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
make \

## Running the demo
./parserDemo pathToNodesetFile1 pathToNodesetFile2
  
## Integration with open62541
take a look on https://github.com/matkonnerth/openWrapper or on the server example

## todos

import of custom types



