/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implementation of a red-black tree with bottom-up insertion and deletion.
 * Much credit for the code goes to Julienne Walker and the Eternally
 * Confuzzled team for creating an excellent tutorial on the data structures:
 *
 * http://www.eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
 * 
 * Almost all of the code is taken from their implementation, with a few tweaks
 * to have nodes hold obl_object structures, and to use enums for direction and
 * color instead of boolean values.
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

static struct iterator_stack {

    struct obl_rb_node *current;

    int seen;

    struct iterator_stack *parent;

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

static struct obl_rb_node *lookup_r(struct obl_rb_node *n,
        obl_set_key key);

static struct obl_rb_node *remove_r(struct obl_rb_node *n,
        obl_set_key key, int *done);

static struct obl_rb_node *remove_balance(struct obl_rb_node *n,
        enum direction dir, int *done);

static struct obl_object *inorder_iternext(struct obl_set_iterator *iter);

static void destroy_r(struct obl_rb_node *n, obl_set_callback callback);

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
    struct obl_rb_node *n;

    n = lookup_r(set->root, key);
    return n == NULL ? NULL : n->o;
}

void obl_set_remove(struct obl_set *set, struct obl_object *o)
{
    int done;

    done = 0;
    set->root = remove_r(set->root, set->keyfunction(o), &done);
}

struct obl_set_iterator *obl_set_inorder_iter(struct obl_set *set)
{
    struct obl_set_iterator *it;
    struct iterator_stack *stack;

    it = malloc(sizeof(struct obl_set_iterator));
    it->next_function = &inorder_iternext;

    stack = malloc(sizeof(struct iterator_stack));
    stack->current = set->root;
    stack->dir = LEFT;
    stack->parent = NULL;

    it->stack = stack;

    return it;
}

struct obl_object *obl_set_iternext(struct obl_set_iterator *iter)
{
    return iter->next_function(iter);
}

struct obl_set_iterator *obl_set_destroyiter(struct obl_set_iterator *iter)
{
    struct iterator_stack *stack, *next;

    stack = iter->stack;
    while (stack != NULL) {
        next = stack->parent;
        free(stack);
        stack = next;
    }

    free(iter);
}

void obl_destroy_set(struct obl_set *set, obl_set_callback callback)
{
    destroy_r(set->root, callback);
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
        /* This key already exists within the set.  Deallocate the old one's
         * storage and assign it the new obl_object payload.
         */
        if (n->o != created->o) {
            _obl_deallocate_object(n->o);
        }
        n->o = created->o;
        free(created);
    }

    return n;
}

static struct obl_rb_node *lookup_r(struct obl_rb_node *n,
        obl_set_key key)
{
    enum direction dir;

    if (n == NULL) {
        return NULL;
    } else if (n->key == key) {
        return n;
    } else {
        return lookup_r(n->children[key < n->key ? LEFT : RIGHT], key);
    }
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

static struct obl_object *inorder_iternext(struct obl_set_iterator *iter)
{
    struct iterator_stack *stack_bottom;
    struct obl_rb_node *current_node;
    struct obl_result *result;

    stack_bottom = iter->stack;
    current_node = stack_bottom->current;

    /*
     * Descend to the beginning of the left subtree, pushing stack frames as you
     * go.
     */
    while (current_node->children[LEFT] != NULL) {
        struct iterator_stack *context;

        current_node = current_node->children[LEFT];

        /* Push a new frame on the stack. */
        context = malloc(sizeof(iterator_stack));
        context->current = current_node;
        context->seen = 0;
        context->parent = stack_bottom;

        stack_bottom = context;
    }

    /* This is going to be the current result. */
    result = current_node->o;

    /* If this node has a right subchild, */

    /* *** */

    if (stack_bottom->seen) {
        struct iterator_stack *context;

        /* Unwind all of the stack that we've already traversed. */
        while (stack_bottom->seen || current_node->children[RIGHT] == NULL) {
            struct iterator_stack *parent;

            /*
             * Pop the top frame from the stack.  If we run out of stack,
             * we're out of set.
             */
            parent = stack_bottom->parent;
            free(stack_bottom);
            if (parent == NULL) {
                return NULL;
            }
            stack_bottom = parent;
            current_node = parent->current;
        }

        /*
         * Begin traversal of the right subtree by pushing a stack frame for the
         * right child.
         */
        context = malloc(sizeof(iterator_stack));
        context->current = current_node->children[RIGHT];
        context->seen = 0;
        context->parent = stack_bottom;
    }

    /*
     * Descend to the beginning of the left subtree, creating stack frames
     * as you go.
     */
    while (current_node->children[LEFT] != NULL) {
        struct iterator_stack *context;

        current_node = current_node->children[LEFT];

        /* Push a new frame on the stack. */
        context = malloc(sizeof(iterator_stack));
        context->current = current_node;
        context->seen = 0;
        context->parent = stack_bottom;

        stack_bottom = context;
    }

    /*
     * We've hit the bottom of the left subtree; swap directions and return
     * this node's value.
     */
    stack_bottom->seen = 1;
    return current_node->o;
}

static void destroy_r(struct obl_rb_node *n, obl_set_callback callback)
{
    if (n == NULL) {
        return ;
    }

    destroy_r(n->children[LEFT], callback);
    destroy_r(n->children[RIGHT], callback);

    if (callback != NULL) {
        (*callback)(n->o);
    }
    free(n);
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
