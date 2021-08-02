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
    NL_FileContext handler;
    handler.addNamespace = addNamespace;
    handler.userContext = &maxValueRank;

    NodesetLoader *loader = NodesetLoader_new(NULL, NULL);

    for (int cnt = 1; cnt < argc; cnt++)
    {
        handler.file = argv[cnt];
        if (!NodesetLoader_importFile(loader, &handler))
        {
            printf("nodeset could not be loaded, exit\n");
            return 1;
        }
    }

    NodesetLoader_sort(loader);

    for (int i = 0; i < NODECLASS_COUNT; i++)
    {
        NodesetLoader_forEachNode(loader, (NL_NodeClass)i, NULL, (NodesetLoader_forEachNode_Func)dumpNode);
    }

    NodesetLoader_delete(loader);

    printf("maxValue Rank: %d", maxValueRank);
    return 0;
}
