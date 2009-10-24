/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "CUnit/Basic.h"

#include "allocator.h"

#include "database.h"
#include "storage/object.h"
#include "platform.h"

static const char *filename = "allocator.obl";

static struct obl_database *setup_database()
{
    struct obl_database *d;
    struct obl_object *allocator, *logical, *physical;

    d = obl_create_database(filename);

    allocator = obl_create_slotted(obl_at_address(d, OBL_ALLOCATOR_SHAPE_ADDR));
    allocator->logical_address = (obl_logical_address) 1;
    logical = obl_create_integer(d, (obl_int) 2);
    physical = obl_create_integer(d, (obl_int) 1);

    obl_slotted_at_put(allocator, (obl_uint) 0, logical);
    obl_slotted_at_put(allocator, (obl_uint) 1, physical);

    d->root.allocator_addr = allocator->logical_address;
    obl_cache_insert(d->cache, allocator);

    return d;
}

static void teardown_database(struct obl_database *d)
{
    struct obl_object *allocator;

    allocator = obl_at_address(d, d->root.allocator_addr);
    obl_destroy_object(obl_slotted_at(allocator, 0));
    obl_destroy_object(obl_slotted_at(allocator, 1));
    obl_destroy_object(allocator);

    obl_destroy_database(d);
}

void test_allocate_logical(void)
{
    struct obl_database *d;
    struct obl_object *allocator, *logical, *physical;
    obl_logical_address result;

    d = setup_database();

    result = obl_allocate_logical(d);
    CU_ASSERT(result == (obl_logical_address) 2);

    result = obl_allocate_logical(d);
    CU_ASSERT(result == (obl_logical_address) 3);

    result = obl_allocate_logical(d);
    CU_ASSERT(result == (obl_logical_address) 4);

    teardown_database(d);
}

void test_allocate_physical(void)
{
    struct obl_database *d;
    obl_physical_address result;

    d = setup_database();

    result = obl_allocate_physical(d, (obl_uint) 10);
    CU_ASSERT(result == (obl_physical_address) 1);

    result = obl_allocate_physical(d, (obl_uint) 5);
    CU_ASSERT(result == (obl_physical_address) 11);

    result = obl_allocate_physical(d, (obl_uint) 256);
    CU_ASSERT(result == (obl_physical_address) 16);

    teardown_database(d);
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

    if (
        (CU_add_test(pSuite,
                "test_allocate_logical",
                test_allocate_logical) == NULL) ||
        (CU_add_test(pSuite,
                "test_allocate_physical",
                test_allocate_physical) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
