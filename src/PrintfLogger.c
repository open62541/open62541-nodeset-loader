#include "InternalLogger.h"
#include <stdio.h>
#include <stdlib.h>

static void logStdOut(const char *level, const char *message)
{
    printf("NODESETLOADER: %s : %s\n", level, message);
}

static void logDebug(void *context, const char *message)
{
    logStdOut("DEBUG", message);
}

static void logWarning(void *context, const char *message)
{
    logStdOut("WARNING", message);
}

static void logError(void *context, const char *message)
{
    logStdOut("ERROR", message);
}

NodesetLoader_Logger *InternalLogger_new()
{
    NodesetLoader_Logger *logger =
        (NodesetLoader_Logger *)calloc(1, sizeof(NodesetLoader_Logger));
    logger->logDebug = &logDebug;
    logger->logWarning = &logWarning;
    logger->logError = &logError;
    return logger;
}

void InternalLogger_delete(NodesetLoader_Logger *logger) { free(logger); }