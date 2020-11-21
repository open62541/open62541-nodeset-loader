#!/bin/bash
lcov --capture --directory ./build --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
#lcov --remove coverage.info '*/open62541/*' --output-file coverage.info
lcov --remove coverage.info '*tests*' --output-file coverage.info
lcov --remove coverage.info '*conan*' --output-file coverage.info
lcov --list coverage.info
genhtml coverage.info --output-directory coverageHtml
chromium ./coverage/index.html