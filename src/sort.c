/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "sort.h"
#include <assert.h>
#include <error.h>
#include <nodesetLoader/nodesetLoader.h>
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

static node *head = NULL;

static node *zeros = NULL;

struct node *root1 = NULL;

static size_t keyCnt = 0;

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

static bool count_items(node *unused) {
    keyCnt++;
    return false;
}

static bool scan_zeros(node *k) {
    if(k->edgeCount == 0 && k->id) {
        if(head == NULL)
            head = k;
        else
            zeros->qlink = k;

        zeros = k;
    }

    return false;
}

static bool recurse_tree(node *rootNode, bool (*action)(node *)) {
    if(rootNode->left == NULL && rootNode->right == NULL)
        return (*action)(rootNode);
    else {
        if(rootNode->left != NULL)
            if(recurse_tree(rootNode->left, action))
                return true;
        if((*action)(rootNode))
            return true;
        if(rootNode->right != NULL)
            if(recurse_tree(rootNode->right, action))
                return true;
    }

    return false;
}

static void walk_tree(node *rootNode, bool (*action)(node *)) {
    if(rootNode->right)
        recurse_tree(rootNode->right, action);
}

void Sort_init() { root1 = new_node(NULL); }
void Sort_cleanup() {free(root1);}

void Sort_addNode(TNode *data)
{
    node *j = NULL;
    //add node, no matter if there are references on it
    j = search_node(root1, &data->id);
    j->data = data;
    Reference *hierachicalRef = data->hierachicalRefs;
    while(hierachicalRef) {
        if(!hierachicalRef->isForward) {

            node *k = search_node(root1, &hierachicalRef->target);
            record_relation(k, j);
        }
        hierachicalRef = hierachicalRef->next;
    }
}

bool Sort_start(struct Nodeset *nodeset, Sort_SortedNodeCallback callback)
{
    walk_tree(root1, count_items);

    while(keyCnt > 0) {
        walk_tree(root1, scan_zeros);

        while(head) {
            edge *e = head->edges;

            if(head->data != NULL) {
                callback(nodeset, head->data);
            }

            head->id = NULL;
            keyCnt--;

            while(e) {
                e->dest->edgeCount--;
                if(e->dest->edgeCount == 0) {
                    zeros->qlink = e->dest;
                    zeros = e->dest;
                }
                edge *tmp = e;
                e = e->next;
                free(tmp);
            }

            node *tmp = head;
            head = head->qlink;
            free(tmp);
        }
        if(keyCnt > 0) {
            printf("graph contains a loop\n");
            free(root1->left);
            free(root1->right);
            free(root1);
            return false;
        }
    }
    free(root1);
    root1=NULL;
    return true;
}
