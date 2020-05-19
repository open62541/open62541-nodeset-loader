/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 */

#include "NodeContainer.h"
#include "Node.h"
#include <assert.h>
#include <stdlib.h>

NodeContainer *NodeContainer_new(size_t initialSize, bool owner)
{
    NodeContainer *container =
        (NodeContainer *)calloc(1, sizeof(NodeContainer));
    assert(container);
    container->nodes =
        (TNode **)calloc(initialSize, sizeof(void *) * initialSize);
    assert(container->nodes);
    container->size = 0;
    container->capacity = initialSize;
    container->incrementSize = initialSize;
    container->owner = owner;
    return container;
}

void NodeContainer_add(NodeContainer *container, TNode *node)
{
    if (container->size == container->capacity)
    {
        container->nodes = (TNode **)realloc(
            container->nodes,
            (container->size + container->incrementSize) * sizeof(void *));
        assert(container->nodes);
        container->capacity += container->incrementSize;
    }
    container->nodes[container->size] = node;
    container->size++;
}

void NodeContainer_delete(NodeContainer *container)
{
    if (container->owner)
    {
        for (size_t i = 0; i < container->size; i++)
        {
            Node_delete(container->nodes[i]);
        }
    }
    free(container->nodes);
    free(container);
}
