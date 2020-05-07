#ifndef NODESETLOADER_EXTENSION_H
#define NODESETLOADER_EXTENSION_H

struct TNode;
typedef void *(*newExtensionCb)(const struct TNode *);
typedef void (*startExtensionCb)(void *extensionData, const char *name);
typedef void (*endExtensionCb)(void *extensionData, const char *name,
                               char *value);
typedef void (*finishExtensionCb)(void *extensionData);

typedef struct
{
    void *userContext;
    newExtensionCb newExtension;
    startExtensionCb start;
    endExtensionCb end;
    finishExtensionCb finish;
} ExtensionInterface;

#endif