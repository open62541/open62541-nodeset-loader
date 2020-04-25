add_subdirectory(stdout)
if(${ENABLE_BACKEND_OPEN62541})
    add_subdirectory(open62541)
endif()