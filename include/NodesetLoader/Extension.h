#ifndef NODESETLOADER_EXTENSION_H
#define NODESETLOADER_EXTENSION_H

typedef void *(*NodesetLoader_newExtensionCb)(void);
typedef void (*NodesetLoader_startExtensionCb)(void *extensionData,
                                               const char *name,
                                               int nAttributes,
                                               const char **attributes);
typedef void (*NodesetLoader_endExtensionCb)(void *extensionData,
                                             const char *name,
                                             const char *value);
typedef void (*NodesetLoader_finishExtensionCb)(void *extensionData);

typedef struct
{
    void *userContext;
    NodesetLoader_newExtensionCb newExtension;
    NodesetLoader_startExtensionCb start;
    NodesetLoader_endExtensionCb end;
    NodesetLoader_finishExtensionCb finish;
} NodesetLoader_ExtensionInterface;

#endif
