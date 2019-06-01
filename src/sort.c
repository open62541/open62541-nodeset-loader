/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "sort.h"
#include "nodesetLoader.h"
#include <assert.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STREQ(a, b) (strcmp((a), (b)) == 0)

struct edge {
    struct node *dest;
    struct edge *next;
};

struct node {
    const char *str;
    struct node *left, *right;
    int balance;
    struct node *qlink;
    struct edge *edges;
    size_t edgeCount;
    const TNode *data;
};

static struct node *head = NULL;

static struct node *zeros = NULL;

static size_t keyCnt = 0;

static struct node *new_node(const char *str) {
    struct node *k = (struct node *)malloc(sizeof *k);

    k->str = str;
    k->left = k->right = NULL;
    k->balance = 0;

    k->edgeCount = 0;
    k->qlink = NULL;
    k->edges = NULL;
    k->data = NULL;
    return k;
}

static struct node *search_node(struct node *root, const char *str) {
    struct node *p, *q, *r, *s, *t;

    assert(root);

    if(root->right == NULL)
        return (root->right = new_node(str));

    t = root;
    s = p = root->right;

    while(true) {
        int a = strcmp(str, p->str);
        if(a == 0)
            return p;

        if(a < 0)
            q = p->left;
        else
            q = p->right;

        if(q == NULL) {
            q = new_node(str);

            if(a < 0)
                p->left = q;
            else
                p->right = q;

            assert(!STREQ(str, s->str));
            if(strcmp(str, s->str) < 0) {
                r = p = s->left;
                a = -1;
            } else {
                r = p = s->right;
                a = 1;
            }

            while(p != q) {
                assert(!STREQ(str, p->str));
                if(strcmp(str, p->str) < 0) {
                    p->balance = -1;
                    p = p->left;
                } else {
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

static void record_relation(struct node *from, struct node *to) {
    struct edge *e;

    if(!STREQ(from->str, to->str)) {
        to->edgeCount++;
        e = (struct edge *)malloc(sizeof(struct edge));
        e->dest = to;
        e->next = from->edges;
        from->edges = e;
    }
}

static bool count_items(struct node *unused) {
    keyCnt++;
    return false;
}

static bool scan_zeros(struct node *k) {
    if(k->edgeCount == 0 && k->str) {
        if(head == NULL)
            head = k;
        else
            zeros->qlink = k;

        zeros = k;
    }

    return false;
}

static bool recurse_tree(struct node *root, bool (*action)(struct node *)) {
    if(root->left == NULL && root->right == NULL)
        return (*action)(root);
    else {
        if(root->left != NULL)
            if(recurse_tree(root->left, action))
                return true;
        if((*action)(root))
            return true;
        if(root->right != NULL)
            if(recurse_tree(root->right, action))
                return true;
    }

    return false;
}

static void walk_tree(struct node *root, bool (*action)(struct node *)) {
    if(root->right)
        recurse_tree(root->right, action);
}

struct node *root;

void init() { root = new_node(NULL); }

void addNodeToSort(const TNode *data) {
    struct node *j = NULL;
    //add node, no matter if there are references on it
    j = search_node(root, data->id.idString);
    j->data = data;
    Reference *hierachicalRef = data->hierachicalRefs;
    while(hierachicalRef) {
        if(!hierachicalRef->isForward) {

            struct node *k = search_node(root, hierachicalRef->target.idString);
            record_relation(k, j);
        }
        hierachicalRef = hierachicalRef->next;
    }
}

void sort(OnSortCallback callback) {
    walk_tree(root, count_items);

    while(keyCnt > 0) {
        walk_tree(root, scan_zeros);

        while(head) {
            struct edge *e = head->edges;

            if(head->data != NULL) {
                callback(head->data);
            }

            head->str = NULL;
            keyCnt--;

            while(e) {
                e->dest->edgeCount--;
                if(e->dest->edgeCount == 0) {
                    zeros->qlink = e->dest;
                    zeros = e->dest;
                }
                struct edge *tmp = e;
                e = e->next;
                free(tmp);
            }

            struct node *tmp = head;
            head = head->qlink;
            free(tmp);
        }

        if(keyCnt > 0) {
            printf("graph contains a loop\n");
            return;
        }
    }
}