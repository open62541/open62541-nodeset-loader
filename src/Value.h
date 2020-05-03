#include <NodesetLoader/TNodeId.h>
#include <stddef.h>
#include <stdbool.h>
#include <NodesetLoader/NodesetLoader.h>

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
    Data *currentData;
};
typedef struct ParserCtx ParserCtx;

Value *Value_new(const TNode* node);
void Value_start(Value* val, const char* name);
void Value_end(Value* val, const char* name, const char*value);
//void Value_finish(Value* val);