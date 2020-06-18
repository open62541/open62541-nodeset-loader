add_subdirectory(client)
add_subdirectory(reference_server)
add_subdirectory(test_server)

find_program (BASH_PROGRAM bash)

add_test(IntegrationTest_DI ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
    "${CMAKE_BINARY_DIR}/bin/integrationClient"
    "${CMAKE_BINARY_DIR}/bin/Output/DI/"
    "${CMAKE_BINARY_DIR}/bin/refServer_DI" 
    "${CMAKE_BINARY_DIR}/bin/testServer"
    "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")


add_test(IntegrationTest_DI_PLCOpen ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
    "${CMAKE_BINARY_DIR}/bin/integrationClient"
    "${CMAKE_BINARY_DIR}/bin/DI_PLCopen/"
    "${CMAKE_BINARY_DIR}/bin/refServer_DI_PLCopen" 
    "${CMAKE_BINARY_DIR}/bin/testServer"
    "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    "${open62541_NODESET_DIR}/PLCopen/Opc.Ua.Plc.NodeSet2.xml"
    )


add_test(IntegrationTest_DI_Euromap_83_77 ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
    "${CMAKE_BINARY_DIR}/bin/integrationClient"
    "${CMAKE_BINARY_DIR}/bin/DI_Euromap_83_77"
    "${CMAKE_BINARY_DIR}/bin/refServer_DI_Euromap_83_77" 
    "${CMAKE_BINARY_DIR}/bin/testServer"
    "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
    "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
    )



# TODO: continue
#     "${PROJECT_SOURCE_DIR}/nodesets"

