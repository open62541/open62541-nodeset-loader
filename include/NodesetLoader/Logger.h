#ifndef NODESETLOADER_LOGGER_H
#define NODESETLOADER_LOGGER_H

struct NodesetLoader_Logger;
typedef struct NodesetLoader_Logger NodesetLoader_Logger;

typedef void (*NodesetLoader_Logger_logDebug)(void *context,
                                              const char *message);
typedef void (*NodesetLoader_Logger_logWarning)(void *context,
                                                const char *message);
typedef void (*NodesetLoader_Logger_logError)(void *context,
                                              const char *message);

struct NodesetLoader_Logger
{
    void *context;
    NodesetLoader_Logger_logDebug logDebug;
    NodesetLoader_Logger_logDebug logWarning;
    NodesetLoader_Logger_logDebug logError;
};

#endif