/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file test_transaction.c
 */

#include "transaction.h"

#include "storage/integer.h"
#include "database.h"
#include "session.h"
#include "set.h"
#include "unitutilities.h"

#include "CUnit/Basic.h"

void test_ensure_transaction(void)
{
    int created = 0;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t, *same;

    CU_ASSERT(s->current_transaction == NULL);

    t = obl_ensure_transaction(s, &created);
    CU_ASSERT(t != NULL);
    CU_ASSERT(created == 1);
    CU_ASSERT(s->current_transaction == t);
    obl_abort_transaction(t);

    CU_ASSERT(s->current_transaction == NULL);

    t = obl_begin_transaction(s);
    CU_ASSERT(s->current_transaction == t);
    created = 0;
    same = obl_ensure_transaction(s, &created);
    CU_ASSERT(created == 0);
    CU_ASSERT(same == t);
    obl_abort_transaction(t);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_mark_dirty(void)
{
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t;
    struct obl_object *o;

    o = obl_create_integer(42);
    CU_ASSERT(o->session == NULL);

    /*
     * Without a session or a transaction, obl_mark_dirty should be a no-op.
     * Basically, these calls shouldn't segfault.
     */
    obl_mark_dirty(o);
    o->session = s;
    o->logical_address = 200;
    o->physical_address = 1024;
    obl_mark_dirty(o);

    t = obl_begin_transaction(s);
    obl_mark_dirty(o);

    CU_ASSERT(obl_set_includes(t->write_set, o));
    CU_ASSERT(!obl_set_includes(s->read_set, o));

    obl_abort_transaction(t);

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_simple_commit(void)
{
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t;
    struct obl_object *o;

    t = obl_begin_transaction(s);

    o = obl_create_integer(-400);
    o->session = s;
    o->logical_address = 100;
    o->physical_address = 256;
    obl_mark_dirty(o);

    obl_commit_transaction(t);
    CU_ASSERT(readable_logical(d->content[256]) == OBL_INTEGER_SHAPE_ADDR);
    CU_ASSERT(readable_int(d->content[257]) == (obl_int) -400);

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_object_discovery(void)
{
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t;

    struct obl_object *root_shape, *root;
    struct obl_object *one_shape, *one, *a, *b;
    struct obl_object *two;

    char *root_slots[] = { "one", "two" };
    char *one_slots[] = { "a", "b" };

    t = obl_begin_transaction(s);

    root_shape = obl_create_cshape("RootClass", 2, root_slots, OBL_SLOTTED);
    root = obl_create_slotted(root_shape);

    one_shape = obl_create_cshape("OneClass", 2, one_slots, OBL_SLOTTED);
    one = obl_create_slotted(one_shape);

    a = obl_create_cstring("A", 1);
    b = obl_create_integer(42);

    two = obl_create_cstring("b", 1);

    obl_slotted_atcnamed_put(one, "a", a);
    obl_slotted_atcnamed_put(one, "b", b);
    obl_slotted_atcnamed_put(root, "one", one);
    obl_slotted_atcnamed_put(root, "two", two);

    /* Fake the insertion of root. */
    root->session = s;
    obl_mark_dirty(root);

    CU_ASSERT(one->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(a->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(b->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(two->physical_address == OBL_PHYSICAL_UNASSIGNED);

    obl_commit_transaction(t);

    CU_ASSERT(one->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(one->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(one->session == s);
    CU_ASSERT(a->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(a->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(a->session == s);
    CU_ASSERT(b->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(b->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(b->session == s);
    CU_ASSERT(two->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(two->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(two->session == s);
    CU_ASSERT(root_shape->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(root_shape->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(root_shape->session == s);
    CU_ASSERT(one_shape->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(one_shape->logical_address != OBL_LOGICAL_UNASSIGNED);
    CU_ASSERT(one_shape->session == s);

    CU_ASSERT(obl_set_includes(s->read_set, one));
    CU_ASSERT(obl_set_includes(s->read_set, two));
    CU_ASSERT(obl_set_includes(s->read_set, a));
    CU_ASSERT(obl_set_includes(s->read_set, b));
    CU_ASSERT(obl_set_includes(s->read_set, root_shape));
    CU_ASSERT(obl_set_includes(s->read_set, one_shape));

    CU_ASSERT(obl_nil()->session == NULL);
    CU_ASSERT(! obl_set_includes(s->read_set, obl_nil()));

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_auto_mark_dirty(void)
{
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t;

    struct obl_object *root_shape;
    char *slots[] = { "one", "two" };
    struct obl_object *root, *one, *two;

    root_shape = obl_create_cshape("RootShape", 2, slots, OBL_SLOTTED);

    /* Fake-persist the root object and its shape. */
    root = obl_create_slotted(root_shape);
    t = obl_begin_transaction(s);
    root->session = s;
    obl_mark_dirty(root);
    obl_commit_transaction(t);

    /*
     * Put a new object into one of root's slots without an active transaction.
     * This should occur within its own implicit transaction.
     */
    one = obl_create_integer(12);
    CU_ASSERT(one->session == NULL);
    obl_slotted_atcnamed_put(root, "one", one);
    CU_ASSERT(one->session == s);

    /*
     * Put a new object into another slot while there is an active transaction.
     * This should add root to the transaction's write set, but the new
     * object should not be persisted until the transaction commits.
     */
    t = obl_begin_transaction(s);

    two = obl_create_integer(42);
    obl_slotted_atcnamed_put(root, "two", two);
    CU_ASSERT(two->session == NULL);
    CU_ASSERT(obl_set_includes(t->write_set, root));

    obl_commit_transaction(t);
    CU_ASSERT(two->session == s);

    obl_destroy_session(s);
    obl_close_database(d);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_transaction_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("transaction", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    ADD_TEST(test_ensure_transaction);
    ADD_TEST(test_mark_dirty);
    ADD_TEST(test_simple_commit);
    ADD_TEST(test_object_discovery);
    ADD_TEST(test_auto_mark_dirty);

    return pSuite;
}
