/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "Sort.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STREQ(a, b) (strcmp((a), (b)) == 0)

struct S_Edge
{
    struct S_Node *dest;
    struct S_Edge *next;
};

typedef struct S_Edge S_Edge;

struct S_Node
{
    const UA_NodeId *id;
    struct S_Node *left, *right;
    int balance;
    struct S_Node *qlink;
    struct S_Edge *edges;
    size_t edgeCount;
    NL_Node *data;
};

typedef struct S_Node S_Node;

struct SortContext
{
    S_Node *head;
    S_Node *zeros;
    S_Node *root1;
    size_t keyCnt;
};

static S_Node *new_node(const UA_NodeId *id)
{
    S_Node *k = (S_Node *)calloc(1, sizeof(S_Node));
    if(!k)
        return NULL;

    k->id = id;
    k->left = k->right = NULL;
    k->balance = 0;

    k->edgeCount = 0;
    k->qlink = NULL;
    k->edges = NULL;
    k->data = NULL;
    return k;
}

static S_Node *
search_node(S_Node *rootNode, const UA_NodeId *nodeId) {
    if(!rootNode)
        return NULL;

    S_Node *p, *q, *r, *s, *t;

    if(rootNode->right == NULL)
        return (rootNode->right = new_node(nodeId));

    t = rootNode;
    s = p = rootNode->right;

    while (true) {
        UA_Order a = UA_NodeId_order(nodeId, p->id);
        if (a == UA_ORDER_EQ)
            return p;

        if (a == UA_ORDER_LESS)
            q = p->left;
        else
            q = p->right;

        if (q == NULL) {
            q = new_node(nodeId);
            if (a == UA_ORDER_LESS)
                p->left = q;
            else
                p->right = q;

            assert(!UA_NodeId_equal(nodeId, s->id));
            if(UA_NodeId_order(nodeId, s->id) == UA_ORDER_LESS) {
                r = p = s->left;
                a = UA_ORDER_LESS;
            } else {
                r = p = s->right;
                a = UA_ORDER_MORE;
            }

            while (p != q) {
                if (p) {
                    assert(!UA_NodeId_equal(nodeId, p->id));
                    if (UA_NodeId_order(nodeId, p->id) == UA_ORDER_LESS) {
                        p->balance = -1;
                        p = p->left;
                    } else {
                        p->balance = 1;
                        p = p->right;
                    }
                }
            }

            if (s->balance == 0 || s->balance == -a) {
                s->balance += a;
                return q;
            }

            if (r) {
                if (r->balance == a) {
                    p = r;
                    if (a == UA_ORDER_LESS) {
                        s->left = r->right;
                        r->right = s;
                    } else {
                        s->right = r->left;
                        r->left = s;
                    }
                    s->balance = r->balance = 0;
                } else {
                    if (a == UA_ORDER_LESS) {
                        if (r->right) {
                            p = r->right;
                            r->right = p->left;
                            p->left = r;
                            s->left = p->right;
                            p->right = s;
                        }
                    } else {
                        if (r->left) {
                            p = r->left;
                            r->left = p->right;
                            p->right = r;
                            s->right = p->left;
                            p->left = s;
                        }
                    }

                    s->balance = 0;
                    r->balance = 0;
                    if (p) {
                        if (p->balance == a)
                            s->balance = -a;
                        else if (p->balance == -a)
                            r->balance = a;
                        p->balance = 0;
                    }
                }
            }

            if (s == t->right)
                t->right = p;
            else
                t->left = p;
            return q;
        }

        if (q->balance) {
            t = p;
            s = q;
        }

        p = q;
    }
}

static void
record_relation(S_Node *from, S_Node *to) {
    if(UA_NodeId_equal(from->id, to->id))
        return;

    struct S_Edge *e = (S_Edge *)calloc(1, sizeof(S_Edge));
    if(!e)
        return;

    to->edgeCount++;
    e->dest = to;
    e->next = from->edges;
    from->edges = e;
}

static bool count_items(SortContext *ctx, S_Node *unused)
{
    ctx->keyCnt++;
    return false;
}

static bool scan_zeros(SortContext *ctx, S_Node *k)
{
    if (k->edgeCount == 0 && k->id)
    {
        if (ctx->head == NULL)
            ctx->head = k;
        else
            ctx->zeros->qlink = k;

        ctx->zeros = k;
    }

    return false;
}

static bool recurse_tree(SortContext *ctx, S_Node *rootNode,
                         bool (*action)(SortContext *ctx, S_Node *))
{
    if (rootNode->left == NULL && rootNode->right == NULL)
        return (*action)(ctx, rootNode);
    else
    {
        if (rootNode->left != NULL)
            if (recurse_tree(ctx, rootNode->left, action))
                return true;
        if ((*action)(ctx, rootNode))
            return true;
        if (rootNode->right != NULL)
            if (recurse_tree(ctx, rootNode->right, action))
                return true;
    }

    return false;
}

static void walk_tree(SortContext *ctx, S_Node *rootNode,
                      bool (*action)(SortContext *ctx, S_Node *))
{
    if (rootNode->right)
        recurse_tree(ctx, rootNode->right, action);
}

SortContext *Sort_init()
{
    SortContext *ctx = (SortContext *)calloc(1, sizeof(SortContext));
    ctx->root1 = new_node(NULL);
    return ctx;
}

static void cleanupEdges(S_Edge *e)
{
    S_Edge *tmp = e;
    while (tmp)
    {
        S_Edge *next = tmp->next;
        free(tmp);
        tmp = next;
    }
}

static void cleanupSubtree(S_Node *n)
{
    if (!n)
    {
        return;
    }
    cleanupSubtree(n->left);
    cleanupSubtree(n->right);
    cleanupEdges(n->edges);
    free(n);
}

void Sort_cleanup(SortContext *ctx)
{
    if (ctx->root1)
    {
        cleanupSubtree(ctx->root1);
    }
    free(ctx);
}

void Sort_addNode(SortContext *ctx, NL_Node *data) {
    S_Node *j = NULL;
    // add node, no matter if there are references on it
    j = search_node(ctx->root1, &data->id);
    j->data = data;
    NL_Reference *hierachicalRef = data->hierachicalRefs;
    if (hierachicalRef) {
        while (hierachicalRef) {
            S_Node *k = search_node(ctx->root1, &hierachicalRef->target);
            if (!hierachicalRef->isForward) {
                record_relation(k, j);
            } else {
                record_relation(j, k);
            }
            hierachicalRef = hierachicalRef->next;
        }
    }
    // add reference generated by ParentNodeId
    if (NodesetLoader_isInstanceNode(data)) {
        NL_InstanceNode *instanceNode = (NL_InstanceNode *)data;
        S_Node *k = search_node(ctx->root1, &instanceNode->parentNodeId);
        if (k->data) {
            NL_Reference *r = k->data->hierachicalRefs;
            while (r) {
                if (UA_NodeId_equal(&r->target, &data->id)) 
                {
                    NL_Reference *newRef = (NL_Reference *)calloc(1, sizeof(NL_Reference));
                    newRef->isForward = !r->isForward;
                    newRef->target = k->data->id;
                    newRef->refType = r->refType;
                    newRef->next = data->hierachicalRefs;
                    data->hierachicalRefs = newRef;
                    break;
                }
                r = r->next;
            }
        }
    }
}

bool Sort_start(SortContext *ctx, struct Nodeset *nodeset,
                Sort_SortedNodeCallback callback, NodesetLoader_Logger *logger)
{
    walk_tree(ctx, ctx->root1, count_items);

    while (ctx->keyCnt > 0)
    {
        walk_tree(ctx, ctx->root1, scan_zeros);

        while (ctx->head)
        {
            S_Edge *e = ctx->head->edges;

            if (ctx->head->data != NULL)
            {
                callback(nodeset, ctx->head->data);
            }

            ctx->head->id = NULL;
            ctx->keyCnt--;

            while (e)
            {
                e->dest->edgeCount--;
                if (e->dest->edgeCount == 0)
                {
                    ctx->zeros->qlink = e->dest;
                    ctx->zeros = e->dest;
                }
                e = e->next;
            }
            ctx->head = ctx->head->qlink;
        }
        if (ctx->keyCnt > 0)
        {
            if (logger)
            {
                logger->log(logger->context, NODESETLOADER_LOGLEVEL_ERROR,
                            "graph contains a loop, abort");
            }
            return false;
        }
    }
    return true;
}
