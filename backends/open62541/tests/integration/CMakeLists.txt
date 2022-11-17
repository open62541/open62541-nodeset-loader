set(PLCOPEN_NODESET_PATH "${open62541_NODESET_BASE_DIR}/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml")

add_subdirectory(client)
add_subdirectory(reference_server)
add_subdirectory(test_server)

find_program (BASH_PROGRAM bash)

message(WARNING ${CMAKE_CURRENT_BINARY_DIR})

#when open62541 is consumed via conan package, the binary output dir is changed??
if(${ENABLE_CONAN})
    add_test(IntegrationTest_DI ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_BINARY_DIR}/bin/integrationClient"
        "${CMAKE_BINARY_DIR}/bin/DI/"
        "${CMAKE_BINARY_DIR}/bin/refServer_DI" 
        "${CMAKE_BINARY_DIR}/bin/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")


    add_test(IntegrationTest_DI_PLCOpen ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_BINARY_DIR}/bin/integrationClient"
        "${CMAKE_BINARY_DIR}/bin/DI_PLCopen/"
        "${CMAKE_BINARY_DIR}/bin/refServer_DI_PLCopen" 
        "${CMAKE_BINARY_DIR}/bin/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        "${PLCOPEN_NODESET_PATH}"
        )


    add_test(IntegrationTest_DI_Euromap_83_77 ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_BINARY_DIR}/bin/integrationClient"
        "${CMAKE_BINARY_DIR}/bin/DI_Euromap_83_77"
        "${CMAKE_BINARY_DIR}/bin/refServer_DI_Euromap_83_77" 
        "${CMAKE_BINARY_DIR}/bin/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
        )
else()
    add_test(IntegrationTest_DI ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
        "${CMAKE_CURRENT_BINARY_DIR}/Output/DI/"
        "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_DI" 
        "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")


    add_test(IntegrationTest_DI_PLCOpen ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
        "${CMAKE_CURRENT_BINARY_DIR}/Output/DI_PLCopen/"
        "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_DI_PLCopen" 
        "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        "${PLCOPEN_NODESET_PATH}"
        )


    add_test(IntegrationTest_DI_Euromap_83_77 ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
        "${CMAKE_CURRENT_BINARY_DIR}/Output/DI_Euromap_83_77"
        "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_DI_Euromap_83_77" 
        "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
        "${open62541_NODESET_BASE_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
        )

endif()
