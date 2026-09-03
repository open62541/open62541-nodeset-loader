/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "NodesetLoader/NodesetLoader.h"

static char nodeidDump[256];
static char *
printId(const UA_NodeId *id) {
    UA_String myStr = {0};
    UA_NodeId_print(id, &myStr);
    if(myStr.length >= 256)
        myStr.length = 255;
    memcpy(nodeidDump, myStr.data, myStr.length);
    nodeidDump[myStr.length] = 0;
    UA_String_clear(&myStr);
    return nodeidDump;
}

static void
addNamespace(void *userContext, size_t namespaceUrisSize,
             UA_String *namespaceUris, UA_NamespaceMapping *nsMapping) {
    (void)userContext;
    for(size_t i = 0; i < namespaceUrisSize; i++) {
        UA_UInt16 localIdx = 0;
        for(; localIdx < nsMapping->namespaceUrisSize; localIdx++) {
            if(UA_String_equal(&namespaceUris[i],
                               &nsMapping->namespaceUris[localIdx]))
                break;
        }
        if(localIdx == nsMapping->namespaceUrisSize)
            UA_Array_appendCopy((void**)&nsMapping->namespaceUris,
                                &nsMapping->namespaceUrisSize,
                                &namespaceUris[i], &UA_TYPES[UA_TYPES_STRING]);
        UA_Array_appendCopy((void**)&nsMapping->remote2local,
                            &nsMapping->remote2localSize, &localIdx,
                            &UA_TYPES[UA_TYPES_UINT16]);
    }
}

static bool
dumpNode(void *userContext, NL_Node *node) {
    printf("NodeId: %s BrowseName: %.*s DisplayName: %.*s\n", printId(&node->id),
           (int)node->browseName.name.length, node->browseName.name.data,
           (int)node->displayName.text.length, node->displayName.text.data);

    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        printf("\teventNotifier: %s\n", ((const NL_ObjectNode *)node)->eventNotifier);
        break;
    case NODECLASS_VARIABLE:
        printf("\tdatatype: %s\n", printId(&((const NL_VariableNode *)node)->datatype));
        printf("\tvalueRank: %s\n", ((const NL_VariableNode *)node)->valueRank);
        printf("\tarrayDimensions: %s\n",
               ((const NL_VariableNode *)node)->valueRank);
        printf("\tminimumSamplingInterval: %s\n",
               ((const NL_VariableNode *)node)->minimumSamplingInterval);
        break;
    default:
        printf("\n");
        break;
    }
    NL_Reference *ref = node->refs;
    while (ref) {
        printf("Reftype: %s", printId(&ref->refType));
        printf(" target: %s\n", printId(&ref->target));
        ref = ref->next;
    }
    return true;
}

static void
printLog(void *context, UA_LogLevel level, UA_LogCategory category,
         const char *message, va_list args) {
    (void)context;
    (void)level;
    (void)category;
    vprintf(message, args);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("specify nodesetfile as argument. E.g. parserDemo text.xml\n");
        return 1;
    }

    NL_FileContext handler;
    memset(&handler, 0, sizeof(NL_FileContext));
    UA_NamespaceMapping nsMapping = {0};
    handler.addNamespace = addNamespace;
    handler.nsMapping = &nsMapping;

    UA_Logger logger = {.log = printLog};

    NodesetLoader *loader = NodesetLoader_new(&logger);

    for(int cnt = 1; cnt < argc; cnt++) {
        handler.file = argv[cnt];
        if(!NodesetLoader_importFile(loader, &handler)) {
            printf("Nodeset %s could not be loaded\n", argv[cnt]);
            return 1;
        }
    }

    NodesetLoader_sort(loader);
    NodesetLoader_forEachNode(loader, NULL, dumpNode);
    NodesetLoader_delete(loader);

    UA_NamespaceMapping_clear(&nsMapping);
    return 0;
}
