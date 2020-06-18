add_executable(integrationClient client.cpp utils.cpp operator_ov.cpp browse_utils.cpp sort_utils.cpp)
target_include_directories(integrationClient PRIVATE client)
target_link_libraries(integrationClient PRIVATE open62541)
