/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "CUnit/Basic.h"

#include "allocator.h"

#include "storage/object.h"
#include "database.h"
#include "set.h"
#include "session.h"
#include "platform.h"
#include "unitutilities.h"

static struct obl_session *setup_session()
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *allocator, *logical, *physical;

    d = obl_open_defdatabase(NULL);
    s = obl_create_session(d);

    allocator = obl_create_slotted(
            _obl_at_fixed_address(OBL_ALLOCATOR_SHAPE_ADDR));
    allocator->logical_address = (obl_logical_address) 1;
    allocator->session = s;
    logical = obl_create_integer((obl_int) 2);
    physical = obl_create_integer((obl_int) 1);

    obl_slotted_at_put(allocator, (obl_uint) 0, logical);
    obl_slotted_at_put(allocator, (obl_uint) 1, physical);

    d->root.allocator_addr = allocator->logical_address;
    obl_set_insert(s->read_set, allocator);

    return s;
}

static void teardown_session(struct obl_session *s)
{
    struct obl_database *d = s->database;
    struct obl_object *allocator;

    allocator = obl_at_address(s, d->root.allocator_addr);
    obl_destroy_object(obl_slotted_at(allocator, 0));
    obl_destroy_object(obl_slotted_at(allocator, 1));

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_allocate_logical(void)
{
    struct obl_session *s;
    obl_logical_address result;

    s = setup_session();

    result = obl_allocate_logical(s);
    CU_ASSERT(result == (obl_logical_address) 2);

    result = obl_allocate_logical(s);
    CU_ASSERT(result == (obl_logical_address) 3);

    result = obl_allocate_logical(s);
    CU_ASSERT(result == (obl_logical_address) 4);

    teardown_session(s);
}

void test_allocate_physical(void)
{
    struct obl_session *s;
    obl_physical_address result;

    s = setup_session();

    result = obl_allocate_physical(s, (obl_uint) 10);
    CU_ASSERT(result == (obl_physical_address) 1);

    result = obl_allocate_physical(s, (obl_uint) 5);
    CU_ASSERT(result == (obl_physical_address) 11);

    result = obl_allocate_physical(s, (obl_uint) 256);
    CU_ASSERT(result == (obl_physical_address) 16);

    teardown_session(s);
}


/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_allocator_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("allocator", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    ADD_TEST(test_allocate_logical);
    ADD_TEST(test_allocate_physical);

    return pSuite;
}
