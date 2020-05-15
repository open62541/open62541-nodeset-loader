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
    ck_assert(!strcmp(val->type, "Double"));
    Value_delete(val);
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
    Value_start(val, "DataType");
    Value_end(val, "DataType", "i=12");
    Value_end(val, "Argument", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    ck_assert(val);
    ck_assert(!strcmp(val->type, "i=297"));
    ck_assert(val->data);
    ck_assert(val->isExtensionObject);
    ck_assert(val->data->type == DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.membersSize == 2);
    ck_assert(!strcmp(val->data->val.complexData.members[0]->name, "Name"));
    ck_assert(!strcmp(val->data->val.complexData.members[1]->name, "DataType"));
    Value_delete(val);
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
    ck_assert(!strcmp(val->type, "UInt32"));
    ck_assert(val->data->type == DATATYPE_COMPLEX);
    ck_assert(!strcmp(val->data->name, "ListOfUInt32"));
    ck_assert(val->data->val.complexData.membersSize == 2);
    ck_assert(!strcmp(
        val->data->val.complexData.members[0]->val.primitiveData.value, "120"));
    ck_assert(!strcmp(
        val->data->val.complexData.members[1]->val.primitiveData.value, "130"));
    Value_delete(val);
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
    // obj1
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
    // obj2
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

    ck_assert(val);
    ck_assert(!strcmp(val->data->name, "ListOfExtensionObject"));
    ck_assert(val->isArray);
    ck_assert(val->isExtensionObject); //?
    ck_assert(val->data->type = DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.membersSize == 2);
    ck_assert(!strcmp(val->data->val.complexData.members[0]->name, "Argument"));
    ck_assert(!strcmp(val->data->val.complexData.members[1]->name, "Argument"));
    Value_delete(val);
}
END_TEST

START_TEST(LocalizedText)
{
    /*
    <Value>
      <LocalizedText>
        <Locale>en</Locale>
        <Text>someText@42</Text>
      </LocalizedText>
    </Value>
    */

    Value *val = Value_new(NULL);
    Value_start(val, "LocalizedText");
    Value_start(val, "Locale");
    Value_end(val, "Locale", "en");
    Value_start(val, "Text");
    Value_end(val, "Text", "someText@42");
    Value_end(val, "LocalizedText", NULL);
    ck_assert(val);
    ck_assert(val->data->type == DATATYPE_COMPLEX);
    ck_assert(!strcmp(val->data->name, "LocalizedText"));
    ck_assert(val->data->val.complexData.membersSize == 2);
    ck_assert(!strcmp(
        val->data->val.complexData.members[0]->val.primitiveData.value, "en"));
    ck_assert(
        !strcmp(val->data->val.complexData.members[1]->val.primitiveData.value,
                "someText@42"));
    Value_delete(val);
}
END_TEST

START_TEST(EnumValueType)
{
/*
<uax:ListOfExtensionObject>
    <uax:ExtensionObject>
        <uax:TypeId>
            <uax:Identifier>i=7616</uax:Identifier>
        </uax:TypeId>
        <uax:Body>
            <uax:EnumValueType>
                <uax:Value>0</uax:Value>
                <uax:DisplayName>
                    <uax:Text>LOG_ON</uax:Text>
                </uax:DisplayName>
                <uax:Description>
                    <uax:Text>The user has logged on the machine</uax:Text>
                </uax:Description>
            </uax:EnumValueType>
        </uax:Body>
    </uax:ExtensionObject>
    <uax:ExtensionObject>
        <uax:TypeId>
            <uax:Identifier>i=7616</uax:Identifier>
        </uax:TypeId>
        <uax:Body>
            <uax:EnumValueType>
                <uax:Value>1</uax:Value>
                <uax:DisplayName>
                    <uax:Text>LOG_OFF</uax:Text>
                </uax:DisplayName>
                <uax:Description>
                    <uax:Text>The user has logged off the machine</uax:Text>
                </uax:Description>
            </uax:EnumValueType>
        </uax:Body>
    </uax:ExtensionObject>
</uax:ListOfExtensionObject>
*/
    Value *val = Value_new(NULL);
    Value_start(val, "ListOfExtensionObject");
    // obj1
    Value_start(val, "ExtensionObject");
    Value_start(val, "TypeId");
    Value_start(val, "Identifier");
    Value_end(val, "Identifier", "i=7616");
    Value_end(val, "TypeId", NULL);
    Value_start(val, "Body");
    Value_start(val, "EnumValueType");
    Value_start(val, "Value");
    Value_end(val, "Value", "0");
    Value_start(val, "DisplayName");
    Value_start(val, "Text");
    Value_end(val, "Text", "LOG_ON");
    Value_end(val, "DisplayName", NULL);
    Value_end(val, "EnumValueType", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    // obj2
    Value_start(val, "ExtensionObject");
    Value_start(val, "TypeId");
    Value_start(val, "Identifier");
    Value_end(val, "Identifier", "i=7616");
    Value_end(val, "TypeId", NULL);
    Value_start(val, "Body");
    Value_start(val, "EnumValueType");
    Value_start(val, "Value");
    Value_end(val, "Value", "1");
    Value_start(val, "DisplayName");
    Value_start(val, "Text");
    Value_end(val, "Text", "LOG_ON");
    Value_end(val, "DisplayName", NULL);
    Value_end(val, "EnumValueType", NULL);
    Value_end(val, "Body", NULL);
    Value_end(val, "ExtensionObject", NULL);
    //
    Value_end(val, "ListOfExtensionObject", NULL);

    ck_assert(val);
    ck_assert(!strcmp(val->data->name, "ListOfExtensionObject"));
    ck_assert(val->isArray);
    ck_assert(val->isExtensionObject); //?
    ck_assert(val->data->type = DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.membersSize == 2);
    ck_assert(!strcmp(val->data->val.complexData.members[0]->name, "EnumValueType"));
    ck_assert(!strcmp(val->data->val.complexData.members[1]->name, "EnumValueType"));
    ck_assert(val->data->val.complexData.members[1]->type == DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.members[1]->val.complexData.membersSize == 2);
    ck_assert(
        val->data->val.complexData.members[1]->val.complexData.members[1]->type = DATATYPE_COMPLEX);
    ck_assert(val->data->val.complexData.members[1]
                  ->val.complexData.members[1]->val.complexData.membersSize==1);
    ck_assert(!strcmp(val->data->val.complexData.members[1]
                  ->val.complexData.members[1]
                  ->val.complexData.members[0]->name, "Text"));
    Value_delete(val);
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
    tcase_add_test(tc, LocalizedText);
    tcase_add_test(tc, EnumValueType);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : -1;
}
