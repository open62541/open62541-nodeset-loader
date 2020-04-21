
#ifndef VALUE_H
#define VALUE_H
#include <open62541/types.h>

struct TNode;
enum VALUE_STATE
{
    VALUE_STATE_DATA,
    VALUE_STATE_FINISHED,
    VALUE_STATE_BUILTIN,
    VALUE_STATE_NODEID,
    VALUE_STATE_LOCALIZEDTEXT,
    VALUE_STATE_QUALIFIEDNAME,
    VALUE_STATE_ERROR
};

struct TypeList;

struct Value
{
    bool isArray;
    enum VALUE_STATE state;
    void *value;
    size_t arrayCnt;
    struct TypeList *typestack;
    size_t offset;
    const char *name;
    const UA_DataType *datatype;
};


struct Value *Value_new(const struct TNode *node);
void Value_start(struct Value *val, const char *localname);
void Value_end(struct Value *val, const char *localname, char *value);
void Value_finish(struct Value *val);
void Value_delete(struct Value **val);

#endif
