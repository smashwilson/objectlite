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
    CU_ASSERT(obl_database_ok(database));

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

    CU_ASSERT(database->log_config.filename == NULL);
    CU_ASSERT(database->log_config.level == L_DEBUG);

    CU_ASSERT(database->default_stub_depth == 4);

    CU_ASSERT(obl_database_ok(database));

    obl_destroy_database(database);
}

void test_report_error(void)
{
    struct obl_database *database;

    database = obl_create_database("foo.obl");
    CU_ASSERT_FATAL(database != NULL);
    CU_ASSERT(obl_database_ok(database));

    database->log_config.level = L_NONE;

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

void test_at_address(void)
{
    struct obl_database *d;
    struct obl_object *in;
    struct obl_object *out;

    d = obl_create_database("foo.obl");

    /* obl_at_address should defer to fixed space when appropriate. */
    out = obl_at_address(d, OBL_TRUE_ADDR);
    CU_ASSERT(obl_boolean_value(out));

    /* obl_at_address should hit the cache. */
    in = obl_create_integer(d, (obl_int) 14);
    in->logical_address = 123;
    obl_cache_insert(d->cache, in);

    out = obl_at_address(d, 123);
    CU_ASSERT(in == out);
    CU_ASSERT(obl_integer_value(out) == (obl_int) 14);

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
                "test_initialize_database",
                test_initialize_database) == NULL) ||
        (CU_add_test(pSuite,
                "test_report_error",
                test_report_error) == NULL) ||
        (CU_add_test(pSuite,
                "test_allocate_fixed_space",
                test_allocate_fixed_space) == NULL) ||
        (CU_add_test(pSuite,
                "test_at_address",
                test_at_address) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
