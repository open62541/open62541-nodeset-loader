/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("specify nodesetfile as argument. E.g. parserDemo text.xml\n");
        return 1;
    }

    int maxValueRank = -1;
    FileContext handler;
    handler.addNamespace = addNamespace;
    handler.userContext = &maxValueRank;
    ValueInterface valIf;
    valIf.userContext = NULL;
    valIf.newValue = Value_new;
    valIf.start = Value_start;
    valIf.end = Value_end;
    valIf.finish = Value_finish;
    valIf.deleteValue = Value_delete;
    handler.valueHandling = &valIf;

    NodesetLoader *loader = NodesetLoader_new();

    for (int cnt = 1; cnt < argc; cnt++)
    {
        handler.file = argv[cnt];
        if (!NodesetLoader_importFile(loader, &handler))
        {
            printf("nodeset could not be loaded, exit\n");
            return 1;
        }
    }

    {
        TReferenceTypeNode **nodes = NULL;
        size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_REFERENCETYPE,
                                            (TNode **)nodes);
        for (TReferenceTypeNode **node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)*node);
        }
    }

    {
        TDataTypeNode *nodes = NULL;
        size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_DATATYPE,
                                            (TNode **)&nodes);
        for (TDataTypeNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
        }
    }

    {
        TObjectTypeNode *nodes = NULL;
        size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_OBJECTTYPE,
                                            (TNode **)&nodes);
        for (TObjectTypeNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
        }
    }

    {
        TObjectNode *nodes = NULL;
        size_t cnt =
            NodesetLoader_getNodes(loader, NODECLASS_OBJECT, (TNode **)&nodes);
        for (TObjectNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
        }
    }

    {
        TMethodNode *nodes = NULL;
        size_t cnt =
            NodesetLoader_getNodes(loader, NODECLASS_METHOD, (TNode **)&nodes);
        for (TMethodNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
        }
    }

    {
        TVariableTypeNode *nodes = NULL;
        size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_VARIABLETYPE,
                                            (TNode **)&nodes);
        for (TVariableTypeNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
        }
    }

    {
        TVariableNode *nodes = NULL;
        size_t cnt = NodesetLoader_getNodes(loader, NODECLASS_VARIABLE,
                                            (TNode **)&nodes);
        for (TVariableNode *node = nodes; node != nodes + cnt; node++)
        {
            dumpNode(NULL, (TNode *)node);
            //BackendOpen62541_Value_delete(&node->value);
        }
    }

    NodesetLoader_delete(loader);

    printf("maxValue Rank: %d", maxValueRank);
    return 0;
}
