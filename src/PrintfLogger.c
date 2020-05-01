#include "InternalLogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static const char* logLevel[3] = {"Debug", "Warning", "Error"};

static void logStdOut(void* context, enum NodesetLoader_LogLevel level, const char *message, ...)
{
    va_list vl;
    va_start(vl, message);
    printf("NODESETLOADER: %s : %s\n", logLevel[level], message);
}

NodesetLoader_Logger *InternalLogger_new()
{
    NodesetLoader_Logger *logger =
        (NodesetLoader_Logger *)calloc(1, sizeof(NodesetLoader_Logger));
    logger->log = &logStdOut;
    return logger;
}

void InternalLogger_delete(NodesetLoader_Logger *logger) { free(logger); }