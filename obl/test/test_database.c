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

    database = obl_create_database(filename);
    CU_ASSERT_FATAL(database != NULL);
    CU_ASSERT(obl_database_ok(database));

    if (database->filename != NULL) {
        CU_ASSERT(strcmp(database->filename, filename) == 0);
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

    database = obl_create_database(filename);
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

    d = obl_create_database(filename);
    CU_ASSERT_FATAL(d != NULL);

    o = _obl_at_address(d, OBL_NIL_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_address(d, OBL_NIL_SHAPE_ADDR));
    CU_ASSERT(o == obl_nil());

    o = _obl_at_address(d, OBL_TRUE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_true());

    o = _obl_at_address(d, OBL_FALSE_ADDR);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->shape != NULL);
    CU_ASSERT(o->shape == _obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR));
    CU_ASSERT(o == obl_false());

    obl_destroy_database(d);
}

void test_at_address(void)
{
    struct obl_database *d;
    struct obl_object *in;
    struct obl_object *out;

    d = obl_create_database(filename);

    /* _obl_at_address should defer to fixed space when appropriate. */
    out = _obl_at_address(d, OBL_TRUE_ADDR);
    CU_ASSERT(obl_boolean_value(out));

    /* _obl_at_address should hit the read set. */
    in = obl_create_integer((obl_int) 14);
    in->logical_address = 123;
    obl_set_insert(d->read_set, in);

    out = _obl_at_address(d, 123);
    CU_ASSERT(in == out);
    CU_ASSERT(obl_integer_value(out) == (obl_int) 14);

    obl_destroy_database(d);
}

void test_open_database(void)
{
    struct obl_database *d;

    remove(filename);

    d = obl_create_database(filename);
    d->log_config.level = L_NONE;
    CU_ASSERT(!obl_is_open(d));

    obl_open_database(d, 0);
    CU_ASSERT(!obl_is_open(d));

    obl_open_database(d, 1);

    CU_ASSERT(obl_is_open(d));
    CU_ASSERT(d->content_size == DEFAULT_GROWTH_SIZE);
    CU_ASSERT(d->content != NULL);

    obl_close_database(d);
    CU_ASSERT(!obl_is_open(d));

    obl_destroy_database(d);
}

void test_database_io(void)
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *o;
    obl_logical_address addr;

    remove(filename);

    /* Write an object into the database. */
    d = obl_create_database(filename);
    obl_open_database(d, 1);
    s = obl_create_session(d);

    o = obl_create_integer((obl_int) 42);
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED);

    o->session = s;
    _obl_write(o);
    CU_ASSERT(o->physical_address != OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->logical_address != OBL_LOGICAL_UNASSIGNED);
    addr = o->logical_address;

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
    obl_destroy_database(d);

    /* Read the object back from the database, clean cache and everything. */
    d = obl_create_database(filename);
    obl_open_database(d, 0);
    s = obl_create_session(d);

    o = obl_at_address(s, addr);
    CU_ASSERT(obl_integer_value(o) == (obl_int) 42);

    obl_close_database(d);
    obl_destroy_session(s);
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

    ADD_TEST(test_initialize_database);
    ADD_TEST(test_report_error);
    ADD_TEST(test_allocate_fixed_space);
    ADD_TEST(test_at_address);
    ADD_TEST(test_open_database);
    ADD_TEST(test_database_io);

    return pSuite;
}
