# nodesetLoader [![Build Status](https://travis-ci.org/matkonnerth/nodesetLoader.svg?branch=master)](https://travis-ci.org/matkonnerth/nodesetLoader)
nodesetLoader is a library written in C99 for the purpose of loading OPC UA nodesets from xml and sorting the nodes based on their hierachical references.

## Current status
This is only a proof of concept, not meant for production use.

## dependencies
xmlImport: libXml (http://www.xmlsoft.org/) for parsing the nodeset xml \
unit testing: Check (https://github.com/libcheck/check)

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

## todos

import of values



