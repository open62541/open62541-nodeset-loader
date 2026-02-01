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

char nodeidDump[256];
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

static UA_NamespaceMapping _nsMapping;

void _addNamespace(void *userContext,
                   size_t localNamespaceUrisSize,
                   UA_String *localNamespaceUris,
                   UA_NamespaceMapping *nsMapping) {
    // Already known?
    for (size_t i = _nsMapping.namespaceUrisSize;
         i < _nsMapping.namespaceUrisSize; i++) {
        if (UA_String_equal(localNamespaceUris, &_nsMapping.namespaceUris[i]))
            return;
    }

    UA_UInt16 localIdx = (UA_UInt16)_nsMapping.namespaceUrisSize;

    // Add to the local mapping
    UA_StatusCode res =
        UA_Array_appendCopy((void**)&_nsMapping.namespaceUris,
                            &_nsMapping.namespaceUrisSize,
                            localNamespaceUris, &UA_TYPES[UA_TYPES_STRING]);
    (void)res;


    res = UA_Array_appendCopy((void**)&_nsMapping.remote2local,
                              &_nsMapping.remote2localSize,
                              &localIdx, &UA_TYPES[UA_TYPES_UINT16]);
    (void)res;
}

bool dumpNode(void *userContext, const NL_Node *node) {
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
NodesetLoader_Logger_printf(void *context,
                          enum NodesetLoader_LogLevel level,
                          const char *message, ...) {
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("specify nodesetfile as argument. E.g. parserDemo text.xml\n");
        return 1;
    }

    NL_FileContext handler;
    memset(&handler, 0, sizeof(NL_FileContext));
    handler.addNamespace = _addNamespace;
    handler.nsMapping = &_nsMapping;

    NodesetLoader_Logger logger;
    logger.log = NodesetLoader_Logger_printf;

    NodesetLoader *loader = NodesetLoader_new(&logger);

    for(int cnt = 1; cnt < argc; cnt++) {
        handler.file = argv[cnt];
        if(!NodesetLoader_importFile(loader, &handler)) {
            printf("Nodeset %s could not be loaded\n", argv[cnt]);
            return 1;
        }
    }

    NodesetLoader_sort(loader);
    NodesetLoader_forEachNode(loader, NULL, (NodesetLoader_forEachNode_Func)dumpNode);
    NodesetLoader_delete(loader);

    UA_NamespaceMapping_clear(&_nsMapping);
    return 0;
}
