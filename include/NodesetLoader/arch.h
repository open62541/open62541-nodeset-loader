#ifndef NODESETLOADER_ARCH_H
#define NODESETLOADER_ARCH_H

#if __GNUC__ || __clang__
#define LOADER_EXPORT __attribute__((visibility("default")))
#endif
#ifndef LOADER_EXPORT
#define LOADER_EXPORT /* fallback to default */
#endif

#endif
