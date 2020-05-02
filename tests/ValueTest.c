#include "../src/Value.h"
#include <check.h>

START_TEST(simpleVal)
{
    //<Value><Double> 3.1415 < / Double > </ Value>

    Value *val = Value_new(NULL);
    Value_start(val, "Double");
    Value_end(val, "Double", "3.1415");
    ck_assert(val);
    ck_assert(val->data->type == DATATYPE_PRIMITIVE);
    ck_assert(!strcmp(val->data->val.primitiveData.value, "3.1415"));
    ck_assert(!strcmp(val->data->name, "Double"));
}
END_TEST

START_TEST(ExtensionObject)
{
    /* ExtensionObject
    <Value>
        <ExtensionObject>
          <TypeId>
            <Identifier>i=297</Identifier>
          </TypeId>
          <Body>
            <Argument>
              <Name>Context</Name>
              <DataType>
                <Identifier>i=12</Identifier>
              </DataType>
              <ValueRank>-1</ValueRank>
              <ArrayDimensions />
            </Argument>
          </Body>
        </ExtensionObject>
    </Value>
*/

    Value *val = Value_new(NULL);
    Value_start(val, "ExtensionObject");
    Value_start(val, "TypeId");
    Value_start(val, "Identifier");
    Value_end(val, "Identifier", "i=297");
    Value_end(val, "TypeId", NULL);
    Value_start(val, "Body");
    Value_start(val, "Argument");
    Value_start(val, "Name");
    Value_end(val, "Name", "Context");
    Value_end(val, "Argument", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    ck_assert(val);
}
END_TEST

START_TEST(ListOfUInt32)
{
    /*
    <Value>
          <ListOfUInt32>
            <UInt32>120</UInt32>
            <UInt32>130</UInt32>
            <UInt32>140</UInt32>
          </ListOfUInt32>
        </Value>
    */

    Value *val = Value_new(NULL);
    Value_start(val, "ListOfUInt32");
    Value_start(val, "UInt32");
    Value_end(val, "UInt32", "120");
    Value_start(val, "UInt32");
    Value_end(val, "UInt32", "130");
    Value_end(val, "ListOfUInt32", NULL);
    ck_assert(val);
    ck_assert(val->data->type == DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.membersSize==2);
    ck_assert(!strcmp(val->data->val.complexData.members[0]->val.primitiveData.value, "120"));
    ck_assert(!strcmp(
        val->data->val.complexData.members[1]->val.primitiveData.value, "130"));
}
END_TEST

START_TEST(ListOfExtensionObject)
{
    /* ListOf
<Value>
      <ListOfExtensionObject
xmlns="http://opcfoundation.org/UA/2008/02/Types.xsd"> <ExtensionObject>
          <TypeId>
            <Identifier>i=297</Identifier>
          </TypeId>
          <Body>
            <Argument>
              <Name>Context</Name>
              <DataType>
                <Identifier>i=12</Identifier>
              </DataType>
              <ValueRank>-1</ValueRank>
              <ArrayDimensions />
            </Argument>
          </Body>
        </ExtensionObject>
        <ExtensionObject>
            <TypeId>
                <Identifier>i=297</Identifier>
            </TypeId>
            <Body>
                <Argument>
                <Name>Arg2</Name>
                <DataType>
                    <Identifier>i=12</Identifier>
                </DataType>
                <ValueRank>-1</ValueRank>
                <ArrayDimensions />
                </Argument>
            </Body>
        </ExtensionObject>
      </ListOfExtensionObject>
*/

    Value *val = Value_new(NULL);
    Value_start(val, "ListOfExtensionObject");
    //obj1
    Value_start(val, "ExtensionObject");
    Value_start(val, "TypeId");
    Value_start(val, "Identifier");
    Value_end(val, "Identifier", "i=297");
    Value_end(val, "TypeId", NULL);
    Value_start(val, "Body");
    Value_start(val, "Argument");
    Value_start(val, "Name");
    Value_end(val, "Name", "Context");
    Value_end(val, "Argument", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    //obj2
    Value_start(val, "ExtensionObject");
    Value_start(val, "TypeId");
    Value_start(val, "Identifier");
    Value_end(val, "Identifier", "i=297");
    Value_end(val, "TypeId", NULL);
    Value_start(val, "Body");
    Value_start(val, "Argument");
    Value_start(val, "Name");
    Value_end(val, "Name", "Context");
    Value_end(val, "Argument", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    //
    Value_end(val, "ListOfExtensionObject", NULL);
}
END_TEST


int main(void)
{
    Suite *s = suite_create("Sort tests");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, simpleVal);
    tcase_add_test(tc, ExtensionObject);
    tcase_add_test(tc, ListOfUInt32);
    tcase_add_test(tc, ListOfExtensionObject);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
