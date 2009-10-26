/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "set.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Internal data structures.
 */

enum color {
    RED,
    BLACK
};

enum direction {
    LEFT,
    RIGHT,
    DIRECTION_MAX
};

struct obl_rb_node {

    struct obl_object *o;

    obl_set_key key;

    enum color color;

    struct obl_rb_node *children[DIRECTION_MAX];

};

/*
 * Convenience macros for concise node manipulation and testing.
 */

#define IS_BLACK(n) ((n) == NULL || (n)->color == BLACK)
#define IS_RED(n) ((n) != NULL && (n)->color == RED)
#define OPPOSITE(d) (!(d))

/*
 * Static function prototypes.
 */

static int verify_r(struct obl_rb_node *n);

static struct obl_rb_node *create_node(obl_set_keyfunction keyfunction,
        struct obl_object *o);

static struct obl_rb_node *insert_r(struct obl_rb_node *n,
        struct obl_rb_node *created);

static struct obl_rb_node *remove_r(struct obl_rb_node *n,
        obl_set_key key, int *done);

static struct obl_rb_node *remove_balance(struct obl_rb_node *n,
        enum direction dir, int *done);

static struct obl_rb_node *rotate_single(struct obl_rb_node *fulcrum,
        enum direction dir);

static struct obl_rb_node *rotate_double(struct obl_rb_node *fulcrum,
        enum direction dir);

static void print_node(struct obl_rb_node *n, int indent);

/*
 * External set API.
 */

struct obl_set *obl_create_set(obl_set_keyfunction keyfunction)
{
    struct obl_set *result;

    result = (struct obl_set*) malloc(sizeof(struct obl_set));
    if (result == NULL) {
        return NULL;
    }

    result->root = NULL;
    result->keyfunction = keyfunction;

    return result;
}

int obl_set_verify(struct obl_set *set)
{
    return verify_r(set->root);
}

void obl_set_print(struct obl_set *set)
{
    print_node(set->root, 0);
}

void obl_set_insert(struct obl_set *set, struct obl_object *o)
{
    struct obl_rb_node *created;

    created = create_node(set->keyfunction, o);
    if (created == NULL) {
        return ;
    }

    set->root = insert_r(set->root, created);
    set->root->color = BLACK;
}

struct obl_object *obl_set_lookup(struct obl_set *set, obl_set_key key)
{
    /* */
}

void obl_set_remove(struct obl_set *set, struct obl_object *o)
{
    int done;

    done = 0;
    set->root = remove_r(set->root, set->keyfunction(o), &done);
}

obl_set_key logical_address_keyfunction(struct obl_object *o)
{
    return (obl_set_key) o->logical_address;
}

obl_set_key heap_address_keyfunction(struct obl_object *o)
{
    return (obl_set_key) o;
}

/*
 * Static function definitions.
 */

static int verify_r(struct obl_rb_node *n)
{
    int left_height, right_height;
    struct obl_rb_node *left_child, *right_child;

    if (n == NULL) {
        /* A single leaf has a BLACK height of 1. */
        return 1;
    }

    left_child = n->children[LEFT];
    right_child = n->children[RIGHT];

    /* Test for RED violations: consecutive red nodes. */
    if (IS_RED(n) && (IS_RED(left_child) || IS_RED(right_child))) {
        fprintf(stderr, "RED violation\n");
        return 0;
    }

    /* Test for invalid binary tree: keys out of order. */
    if ( (left_child != NULL && left_child->key >= n->key) ||
            (right_child != NULL && right_child->key <= n->key) ) {
        fprintf(stderr, "Binary Tree violation (%u < (%u) < %u)\n",
                (left_child == NULL ? 0 : left_child->key),
                n->key,
                (right_child == NULL ? 0 : right_child->key));
        return 0;
    }

    left_height = verify_r(left_child);
    right_height = verify_r(right_child);

    /* Test for BLACK violations: mismatched BLACK heights. */
    if (left_height != 0 && right_height != 0 && left_height != right_height) {
        fprintf(stderr, "BLACK violation (%d %d) @ %lu\n",
                left_height, right_height, (unsigned long) n->key);
        return 0;
    }

    /* Only count BLACK nodes. */
    if (left_height != 0 && right_height != 0) {
        return IS_BLACK(n) ? left_height + 1 : left_height;
    } else {
        return 0;
    }
}

static struct obl_rb_node *create_node(obl_set_keyfunction keyfunction,
        struct obl_object *o)
{
    struct obl_rb_node *result;

    result = (struct obl_rb_node *) malloc(sizeof(struct obl_rb_node));
    if (result != NULL) {
        result->color = RED;
        result->children[LEFT] = NULL;
        result->children[RIGHT] = NULL;
        result->o = o;
        result->key = keyfunction(o);
    }

    return result;
}

static struct obl_rb_node *insert_r(struct obl_rb_node *n,
        struct obl_rb_node *created)
{
    if (n == NULL) {
        n = created;
    } else if (created->key != n->key) {
        enum direction dir = created->key > n->key;

        n->children[dir] = insert_r(n->children[dir], created);

        if (IS_RED(n->children[dir])) {
            if (IS_RED(n->children[OPPOSITE(dir)])) {
                /* Case 1: Both children are RED */
                n->color = RED;
                n->children[LEFT]->color = BLACK;
                n->children[RIGHT]->color = BLACK;
            } else {
                /* Case 2: Sibling is BLACK */
                if (IS_RED(n->children[dir]->children[dir])) {
                    n = rotate_single(n, OPPOSITE(dir));
                /* Case 3: Other child is BLACK */
                } else if (IS_RED(n->children[dir]->children[OPPOSITE(dir)])) {
                    n = rotate_double(n, OPPOSITE(dir));
                }
            }
        }

    } else {
        /* This key already exists within the set. */
        /* FIXME: Deallocate n->o in an orderly fashion. */
        n->o = created->o;
        free(created);
    }

    return n;
}

static struct obl_rb_node *remove_r(struct obl_rb_node *n,
        obl_set_key key, int *done)
{
    if (n == NULL) {
        *done = 1;
    } else {
        enum direction dir;

        if (n->key == key) {
            /* This is the node to delete. */
            if (n->children[LEFT] == NULL || n->children[RIGHT] == NULL) {
                struct obl_rb_node *saved;

                /* At most one child is non-NULL. */
                saved = n->children[n->children[LEFT] == NULL];

                /* Removing a red node, which is easy. */
                if (IS_RED(n)) {
                    *done = 1;
                /* Case 0: Removing a node with a single RED child. */
                } else if (IS_RED(saved)) {
                    saved->color = BLACK;
                    *done = 1;
                }

                /* FIXME: Deallocate n->o as well. */
                free(n);

                return saved;
            } else {
                struct obl_rb_node *heir;

                /* Find the in-order predecessor. */
                heir = n->children[LEFT];
                while (heir->children[RIGHT] != NULL) {
                    heir = heir->children[RIGHT];
                }

                /* Swap data with the heir. */
                n->o = heir->o;
                n->key = heir->key;

                key = heir->key;
            }
        }

        dir = (enum direction) n->key < key;
        n->children[dir] = remove_r(n->children[dir], key, done);

        if (!*done) {
            n = remove_balance(n, dir, done);
        }
    }

    return n;
}

static struct obl_rb_node *remove_balance(struct obl_rb_node *n,
        enum direction dir, int *done)
{
    struct obl_rb_node *p = n;
    struct obl_rb_node *s = n->children[OPPOSITE(dir)];

    /* Reduce the cases to consider by removing a RED sibling. */
    if (IS_RED(s)) {
        n = rotate_single(n, dir);
        s = p->children[OPPOSITE(dir)];
    }

    if (s != NULL) {
        if (IS_BLACK(s->children[LEFT]) && IS_BLACK(s->children[RIGHT])) {
            if (IS_RED(p)) {
                *done = 1;
            }
            p->color = BLACK;
            s->color = RED;
        } else {
            enum color n_color;
            int new_root;

            n_color = n->color;
            new_root = (n == p);

            if (IS_RED(s->children[OPPOSITE(dir)])) {
                p = rotate_single(p, dir);
            } else {
                p = rotate_double(p, dir);
            }

            p->color = n_color;
            p->children[LEFT]->color = BLACK;
            p->children[RIGHT]->color = BLACK;

            if (new_root) {
                n = p;
            }

            *done = 1;
        }
    }

    return n;
}

static struct obl_rb_node *rotate_single(struct obl_rb_node *fulcrum,
        enum direction dir)
{
    struct obl_rb_node *saved = fulcrum->children[OPPOSITE(dir)];

    fulcrum->children[OPPOSITE(dir)] = saved->children[dir];
    saved->children[dir] = fulcrum;

    fulcrum->color = RED;
    saved->color = BLACK;

    return saved;
}

static struct obl_rb_node *rotate_double(struct obl_rb_node *fulcrum,
        enum direction dir)
{
    fulcrum->children[OPPOSITE(dir)] =
            rotate_single(fulcrum->children[OPPOSITE(dir)], OPPOSITE(dir));

    return rotate_single(fulcrum, dir);
}

static void print_node(struct obl_rb_node *n, int indent)
{
    int in;

    for (in = 0; in < indent - 2; in++ ) { putchar(' '); }
    if (indent >= 2) { putchar('\\'); }
    if (indent >= 1) { putchar('='); }

    if (n == NULL) {
        printf("B:*\n");
        return ;
    }

    printf("[");
    printf(IS_RED(n) ? "R" : "B");
    printf(":%ld]\n", (unsigned long) n->key);

    print_node(n->children[LEFT], indent + 2);
    print_node(n->children[RIGHT], indent + 2);
}
