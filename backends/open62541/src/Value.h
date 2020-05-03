#ifndef VALUE_H
#define VALUE_H
#include <open62541/types.h>
#include <NodesetLoader/NodesetLoader.h>

struct RawData
{
    void *mem;
    size_t offset;
};
typedef struct RawData RawData;
void RawData_delete(RawData *data);

RawData *Value_getData(const TNode* node, const Value *value);

#endif
