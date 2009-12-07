/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the obl_database struct and the operations provide on it.
 * This will constitute a fair amount of the public API.
 */

#include "CUnit/Basic.h"

#include "storage/object.h"
#include "allocator.h"
#include "constants.h"
#include "database.h"
#include "session.h"
#include "set.h"
#include "unitutilities.h"

#include <stdio.h>

static const char *filename = "database.obl";

void test_initialize_database(void)
{
    struct obl_database *database;

    database = obl_open_defdatabase(filename);
    CU_ASSERT_FATAL(database != NULL);
    CU_ASSERT(obl_database_ok(database));

    if (database->configuration.filename != NULL) {
        CU_ASSERT(strcmp(database->configuration.filename, filename) == 0);
    }

    CU_ASSERT(database->configuration.log_filename == NULL);
    CU_ASSERT(database->configuration.log_level == L_NOTICE);
    CU_ASSERT(database->configuration.default_stub_depth == DEFAULT_STUB_DEPTH);
    CU_ASSERT(database->configuration.growth_size == DEFAULT_GROWTH_SIZE)

    CU_ASSERT(obl_database_ok(database));

    obl_close_database(database);
}

void test_report_error(void)
{
    struct obl_database_config conf = { 0 };
    struct obl_database *database;

    conf.filename = filename;
    conf.log_level = L_NONE;

    remove(filename);
    database = obl_open_database(&conf);

    obl_report_error(database, OBL_OUT_OF_MEMORY, "A sample error message.");
    CU_ASSERT(!obl_database_ok(database));
    CU_ASSERT(strcmp(database->error_message, "A sample error message.")
            == 0);
    CU_ASSERT(database->error_code == OBL_OUT_OF_MEMORY);

    obl_clear_error(database);
    CU_ASSERT(obl_database_ok(database));
    CU_ASSERT(database->error_message == NULL);
    CU_ASSERT(database->error_code == OBL_OK);

    obl_report_error(database, OBL_OUT_OF_MEMORY, NULL);
    CU_ASSERT(strcmp(database->error_message, "Unable to allocate an object") == 0);

    obl_clear_error(database);
    obl_report_errorf(database, OBL_OUT_OF_MEMORY, "%d - %d - %d", 10, 20, 30);
    CU_ASSERT(strcmp(database->error_message, "10 - 20 - 30") == 0);

    obl_close_database(database);
}

void test_allocate_fixed_space(void)
{
    struct obl_object *o;

    o = _obl_at_fixed_address(OBL_NIL_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_fixed_address(OBL_NIL_SHAPE_ADDR));
    CU_ASSERT(o == obl_nil());

    o = _obl_at_fixed_address(OBL_TRUE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_fixed_address(OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_true());

    o = _obl_at_fixed_address(OBL_FALSE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_fixed_address(OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_false());
}

/**
 * Round-trip a simple object through the database using primitive I/O
 * functions.
 */
void test_database_roundtrip(void)
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *o;
    obl_logical_address addr;

    remove(filename);

    /* Write an object into the database. */
    d = obl_open_defdatabase(filename);
    s = obl_create_session(d);

    o = obl_create_integer((obl_int) 42);
    o->session = s;

    _obl_write(o);
    addr = o->logical_address;

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);

    /* Read the object back from the database, clean cache and everything. */
    d = obl_open_defdatabase(filename);
    s = obl_create_session(d);

    o = obl_at_address(s, addr);
    CU_ASSERT(obl_integer_value(o) == (obl_int) 42);

    obl_destroy_session(s);
    obl_close_database(d);
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

    ADD_TEST(test_initialize_database);
    ADD_TEST(test_report_error);
    ADD_TEST(test_allocate_fixed_space);
    ADD_TEST(test_database_roundtrip);

    return pSuite;
}
