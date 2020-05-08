add_subdirectory(client)
add_subdirectory(reference_server)
add_subdirectory(test_server)

find_program (BASH_PROGRAM bash)
add_test(IntegrationTest ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
    "${CMAKE_CURRENT_BINARY_DIR}/reference_server/referenceServer" 
    "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
    "${open62541_NODESET_DIR}"
    "${PROJECT_SOURCE_DIR}/nodesets"
    "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
    "${CMAKE_CURRENT_BINARY_DIR}/Output/")
