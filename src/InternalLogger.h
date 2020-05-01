#ifndef INTERNALLOGGER_H
#define INTERNALLOGGER_H
#include <NodesetLoader/Logger.h>

NodesetLoader_Logger *InternalLogger_new(void);
void InternalLogger_delete(NodesetLoader_Logger *logger);

#endif
