#ifndef REFSERVICEIMPL_H
#define REFSERVICEIMPL_H
#include <NodesetLoader/ReferenceService.h>

struct UA_Server;
RefService *RefServiceImpl_new(struct UA_Server* server);
void RefServiceImpl_delete(RefService *service);
#endif