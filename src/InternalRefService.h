#ifndef INTERNALREFSERVICE_H
#define INTERNALREFSERVICE_H
#include <NodesetLoader/ReferenceService.h>

RefService *InternalRefService_new(void);
void InternalRefService_delete(RefService *service);
#endif
