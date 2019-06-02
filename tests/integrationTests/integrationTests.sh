#!/bin/bash
./parserDemo ../nodesets/testNodeset.xml
./parserDemo ../nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml
./parserDemo ../nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml ../nodesets/UA-Nodeset/DI/Opc.Ua.Di.NodeSet2.xml
./parserDemo ../nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml ../nodesets/UA-Nodeset/DI/Opc.Ua.Di.NodeSet2.xml ../nodesets/UA-Nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml
valgrind --error-exitcode=100 --leak-check=full ./parserDemo ../nodesets/testNodeset100nodes.xml
valgrind --error-exitcode=100 --leak-check=full ./parserDemo ../nodesets/UA-Nodeset/Schema/Opc.Ua.NodeSet2.xml ../nodesets/UA-Nodeset/DI/Opc.Ua.Di.NodeSet2.xml ../nodesets/UA-Nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml