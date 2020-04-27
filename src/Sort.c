/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "Sort.h"
#include <assert.h>
#include <error.h>
#include <nodesetLoader/NodesetLoader.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STREQ(a, b) (strcmp((a), (b)) == 0)

struct edge {
    struct node *dest;
    struct edge *next;
};

typedef struct edge edge;

struct node {
    const TNodeId* id;
    struct node *left, *right;
    int balance;
    struct node *qlink;
    struct edge *edges;
    size_t edgeCount;
    TNode *data;
};

typedef struct node node;

struct SortContext
{
    node *head;
    node *zeros;
    node *root1;
    size_t keyCnt;
};

static node *new_node(const TNodeId* id) {
    node *k = (node *)malloc(sizeof *k);

    k->id = id;
    k->left = k->right = NULL;
    k->balance = 0;

    k->edgeCount = 0;
    k->qlink = NULL;
    k->edges = NULL;
    k->data = NULL;
    return k;
}

static node *search_node(node *rootNode, const TNodeId* nodeId) {
    node *p, *q, *r, *s, *t;

    assert(rootNode);

    if(rootNode->right == NULL)
        return (rootNode->right = new_node(nodeId));

    t = rootNode;
    s = p = rootNode->right;

    while(true) {
        int a = TNodeId_cmp(nodeId, p->id);
        if(a == 0)
            return p;

        if(a < 0)
            q = p->left;
        else
            q = p->right;

        if(q == NULL) {
            q = new_node(nodeId);

            if(a < 0)
                p->left = q;
            else
                p->right = q;

            assert(TNodeId_cmp(nodeId, s->id));
            if (TNodeId_cmp(nodeId, s->id) < 0)
            {
                r = p = s->left;
                a = -1;
            }
            else
            {
                r = p = s->right;
                a = 1;
            }

            while(p != q) {
                assert(TNodeId_cmp(nodeId, p->id));
                if (TNodeId_cmp(nodeId, p->id) < 0)
                {
                    p->balance = -1;
                    p = p->left;
                }
                else
                {
                    p->balance = 1;
                    p = p->right;
                }
            }

            if(s->balance == 0 || s->balance == -a) {
                s->balance += a;
                return q;
            }

            if(r->balance == a) {
                p = r;
                if(a < 0) {
                    s->left = r->right;
                    r->right = s;
                } else {
                    s->right = r->left;
                    r->left = s;
                }
                s->balance = r->balance = 0;
            } else {
                if(a < 0) {
                    p = r->right;
                    r->right = p->left;
                    p->left = r;
                    s->left = p->right;
                    p->right = s;
                } else {
                    p = r->left;
                    r->left = p->right;
                    p->right = r;
                    s->right = p->left;
                    p->left = s;
                }

                s->balance = 0;
                r->balance = 0;
                if(p->balance == a)
                    s->balance = -a;
                else if(p->balance == -a)
                    r->balance = a;
                p->balance = 0;
            }

            if(s == t->right)
                t->right = p;
            else
                t->left = p;

            return q;
        }

        if(q->balance) {
            t = p;
            s = q;
        }

        p = q;
    }
}

static void record_relation(node *from, node *to) {
    struct edge *e;

    if (TNodeId_cmp(from->id, to->id))
    {
        to->edgeCount++;
        e = (edge *)malloc(sizeof(edge));
        e->dest = to;
        e->next = from->edges;
        from->edges = e;
    }
}

static bool count_items(SortContext* ctx, node *unused) {
    ctx->keyCnt++;
    return false;
}

static bool scan_zeros(SortContext* ctx, node *k) {
    if(k->edgeCount == 0 && k->id) {
        if(ctx->head == NULL)
            ctx->head = k;
        else
            ctx->zeros->qlink = k;

        ctx->zeros = k;
    }

    return false;
}

static bool recurse_tree(SortContext* ctx, node *rootNode, bool (*action)(SortContext* ctx, node *)) {
    if(rootNode->left == NULL && rootNode->right == NULL)
        return (*action)(ctx, rootNode);
    else {
        if(rootNode->left != NULL)
            if(recurse_tree(ctx, rootNode->left, action))
                return true;
        if((*action)(ctx, rootNode))
            return true;
        if(rootNode->right != NULL)
            if(recurse_tree(ctx, rootNode->right, action))
                return true;
    }

    return false;
}

static void walk_tree(SortContext* ctx, node *rootNode, bool (*action)(SortContext* ctx, node *)) {
    if(rootNode->right)
        recurse_tree(ctx, rootNode->right, action);
}

SortContext* Sort_init() { 
    SortContext* ctx = (SortContext*) calloc(1, sizeof(SortContext));
    ctx->root1 = new_node(NULL);
    return ctx;
}

static void cleanupEdges(edge* e)
{
    edge* tmp=e;
    while(tmp)
    {
        edge* next = tmp->next;
        free(tmp);
        tmp=next;
    }
}

static void cleanupSubtree(node* n)
{
    if(!n)
    {
        return;
    }
    cleanupSubtree(n->left);
    cleanupSubtree(n->right);
    cleanupEdges(n->edges);
    free(n);
}

void Sort_cleanup(SortContext* ctx) {
    if(ctx->root1)
    {
        cleanupSubtree(ctx->root1);
    }
    free(ctx);
}

void Sort_addNode(SortContext* ctx, TNode *data)
{
    node *j = NULL;
    //add node, no matter if there are references on it
    j = search_node(ctx->root1, &data->id);
    j->data = data;
    Reference *hierachicalRef = data->hierachicalRefs;
    while(hierachicalRef) {
        if(!hierachicalRef->isForward) {

            node *k = search_node(ctx->root1, &hierachicalRef->target);
            record_relation(k, j);
        }
        hierachicalRef = hierachicalRef->next;
    }
}

bool Sort_start(SortContext* ctx, struct Nodeset *nodeset, Sort_SortedNodeCallback callback)
{
    walk_tree(ctx, ctx->root1, count_items);

    while(ctx->keyCnt > 0) {
        walk_tree(ctx, ctx->root1, scan_zeros);

        while(ctx->head) {
            edge *e = ctx->head->edges;

            if(ctx->head->data != NULL) {
                callback(nodeset, ctx->head->data);
            }

            ctx->head->id = NULL;
            ctx->keyCnt--;

            while(e) {
                e->dest->edgeCount--;
                if(e->dest->edgeCount == 0) {
                    ctx->zeros->qlink = e->dest;
                    ctx->zeros = e->dest;
                }
                e = e->next;
            }
            ctx->head = ctx->head->qlink;
        }
        if(ctx->keyCnt > 0) {
            printf("graph contains a loop\n");
            return false;
        }
    }
    return true;
}
