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
    CU_ASSERT(!obl_set_includes(d->read_set, o));

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

    OBL_INFO(d, "Beginning transaction.");
    t = obl_begin_transaction(s);

    OBL_INFO(d, "Creating integer object.");
    o = obl_create_integer(-400);
    o->session = s;
    o->logical_address = 100;
    o->physical_address = 256;
    OBL_INFO(d, "Marking object dirty.");
    obl_mark_dirty(o);

    OBL_INFO(d, "Committing transaction.");
    obl_commit_transaction(t);
    CU_ASSERT(readable_logical(d->content[256]) == OBL_INTEGER_SHAPE_ADDR);
    CU_ASSERT(readable_int(d->content[257]) == (obl_int) -400);

    OBL_INFO(d, "Cleaning up.");
    obl_destroy_object(o);
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

    return pSuite;
}
