#include <NodesetLoader/TNodeId.h>
#include <stddef.h>
#include <stdbool.h>
#include <NodesetLoader/NodesetLoader.h>
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

/* ListOf
<Value>
      <ListOfExtensionObject xmlns="http://opcfoundation.org/UA/2008/02/Types.xsd">
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

/*
<Value>
      <ListOfUInt32>
        <UInt32>120</UInt32>
        <UInt32>130</UInt32>
        <UInt32>140</UInt32>
      </ListOfUInt32>
    </Value>
*/

/*
<Value>
      <LocalizedText>
        <Locale>en</Locale>
        <Text>someText@42</Text>
      </LocalizedText>
    </Value>
*/

struct Data;
typedef struct Data Data;

enum ParserState
{
  PARSERSTATE_INIT,
  PARSERSTATE_LISTOF,
  PARSERSTATE_EXTENSIONOBJECT,
  PARSERSTATE_EXTENSIONOBJECT_TYPEID,
  PARSERSTATE_EXTENSIONOBJECT_BODY,
  PARSERSTATE_DATA,
  PARSERSTATE_FINISHED
};
typedef enum ParserState ParserState;


struct ParserCtx
{
  ParserState state;
  Data* currentData;
};
typedef struct ParserCtx ParserCtx;

enum DataType
{
  DATATYPE_PRIMITIVE,
  DATATYPE_COMPLEX,
};

typedef enum DataType DataType;


struct PrimitiveData
{
    const char* value;
};
typedef struct PrimitiveData PrimitiveData;
struct ComplexData
{
  size_t membersSize;
  Data **members;
};
typedef struct ComplexData ComplexData;

struct Data
{
  DataType type;
  const char *name;
  union
  {
    PrimitiveData primitiveData;
    ComplexData complexData;
  } val;
  Data* parent;
};

struct Value
{
    ParserCtx* ctx;
    bool isArray;
    bool isExtensionObject;
    TNodeId typeId;
    Data* data;
};

Value *Value_new(const TNode* node);
void Value_start(Value* val, const char* name);
void Value_end(Value* val, const char* name, const char*value);
void Value_finish(Value* val);