/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the obl_database struct and the operations provide on it.
 * This will constitute a fair amount of the public API.
 */

#include "CUnit/Headers/Basic.h"

#include "cache.h"
#include "constants.h"
#include "database.h"
#include "object.h"

void test_initialize_database(void)
{
    struct obl_database *database;
    struct obl_cache *cache;

    database = obl_create_database("foo.obl");
    CU_ASSERT_FATAL(database != NULL);

    if (database->filename != NULL) {
        CU_ASSERT(strcmp(database->filename, "foo.obl") == 0);
    }

    if (database->cache != NULL) {
        cache = database->cache;
        CU_ASSERT(cache->current_size == 0);
        CU_ASSERT(cache->max_size == DEFAULT_CACHE_SIZE);
        CU_ASSERT(cache->bucket_count == DEFAULT_CACHE_BUCKETS);
        CU_ASSERT(cache->buckets != NULL);
    } else {
        CU_FAIL("Cache not allocated.");
    }

    CU_ASSERT(obl_database_ok(database));

    obl_destroy_database(database);
}

void test_report_error(void)
{
    struct obl_database *database;

    database = obl_create_database("foo.obl");
    CU_ASSERT_FATAL(database != NULL);
    CU_ASSERT(obl_database_ok(database));

    obl_report_error(database, OBL_OUT_OF_MEMORY, "A sample error message.");

    CU_ASSERT(!obl_database_ok(database));
    CU_ASSERT(strcmp(database->last_error.message, "A sample error message.")
            == 0);
    CU_ASSERT(database->last_error.code == OBL_OUT_OF_MEMORY);

    obl_clear_error(database);

    CU_ASSERT(obl_database_ok(database));
    CU_ASSERT(database->last_error.message == NULL);
    CU_ASSERT(database->last_error.code == OBL_OK);

    obl_report_error(database, OBL_OUT_OF_MEMORY, NULL);
    CU_ASSERT(strcmp(database->last_error.message, "Unable to allocate an object") == 0);

    obl_clear_error(database);
    obl_report_errorf(database, OBL_OUT_OF_MEMORY, "%d - %d - %d", 10, 20, 30);
    CU_ASSERT(strcmp(database->last_error.message, "10 - 20 - 30") == 0);

    obl_destroy_database(database);
}

void test_allocate_fixed_space(void)
{
    struct obl_database *d;
    struct obl_object *o;

    d = obl_create_database("foo.obl");
    CU_ASSERT_FATAL(d != NULL);

    o = obl_at_address(d, OBL_NIL_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_NIL_SHAPE_ADDR));
    CU_ASSERT(o == obl_nil(d));

    o = obl_at_address(d, OBL_TRUE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_true(d));

    o = obl_at_address(d, OBL_FALSE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_false(d));

    obl_destroy_database(d);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_database_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("database", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    if (
        (CU_add_test(pSuite,
                "Initialize the database structure.",
                test_initialize_database) == NULL) ||
        (CU_add_test(pSuite,
                "Reporting and clearing errors.",
                test_report_error) == NULL) ||
        (CU_add_test(pSuite,
                "Fixed space allocation.",
                test_allocate_fixed_space) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
