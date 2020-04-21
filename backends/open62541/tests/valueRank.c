#include <check.h>

std::string path = "";
using opc::NodeId;

TEST(import, valueRank)
{
    opc::Server server;
    ASSERT_TRUE(server.loadNodeset(path + "/" + "valuerank.xml"));


    opc::Variant var;
    ASSERT_TRUE(server.getVariable(NodeId(2, 6002))->read(var));
    ASSERT_TRUE(var.get<int>()==1);
    ASSERT_TRUE(server.getVariable(NodeId(2, 6003))->read(var));
    ASSERT_TRUE(var.get<std::vector<int>>()[1] == 13);
    ASSERT_TRUE(server.getVariable(opc::NodeId(2, 6004))->read(var));
    ASSERT_TRUE(var.get<std::vector<int>>()[2] == 300);
}

int main(int argc, char**argv)
{

    testing::InitGoogleTest(&argc, argv);
    
    if(!(argc>1))
        return 1;
    path = argv[1];


    return RUN_ALL_TESTS();
}