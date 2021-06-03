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
else()
    add_test(IntegrationTest_DI ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
        "${CMAKE_CURRENT_BINARY_DIR}/Output/DI/"
        "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_DI" 
        "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
        "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml")


    add_test(IntegrationTest_Euromap_Instances ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
        "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
        "${CMAKE_CURRENT_BINARY_DIR}/Output/Euromap_Instances"
        "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_DI_Euromap_Instances" 
        "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
        "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/nodesets/euromap_instances/euromapinstances.xml"
        )

    #add_test(IntegrationTest_Struct ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/start_test.sh 
    #    "${CMAKE_CURRENT_BINARY_DIR}/client/integrationClient"
    #    "${CMAKE_CURRENT_BINARY_DIR}/Output/Struct"
    #    "${CMAKE_CURRENT_BINARY_DIR}/reference_server/refServer_Struct" 
    #    "${CMAKE_CURRENT_BINARY_DIR}/test_server/testServer"
    #    "${PROJECT_SOURCE_DIR}/nodesets/struct_union_optionset/structtest.xml"
    #    )

endif()
