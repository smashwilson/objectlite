/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for struct obl_object creation, access, and manipulation API.
 */

#include "storage/object.h"

#include <string.h>
#include <stdlib.h>

#include "CUnit/Basic.h"
#include "unicode/ucnv.h"

#include "database.h"
#include "log.h"
#include "session.h"
#include "set.h"
#include "unitutilities.h"

void test_integer_object(void)
{
    struct obl_object *o;

    o = obl_create_integer((obl_int) 42);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->session == NULL);
    CU_ASSERT(o->shape == _obl_at_fixed_address(OBL_INTEGER_SHAPE_ADDR));
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_integer_value(o) == (obl_int) 42);
    obl_destroy_object(o);
}

void test_string_object(void)
{
    char *string = "NULL-terminated C string.";
    struct obl_object *o;
    char *buffer;

    o = obl_create_cstring(string, strlen(string));
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->session == NULL);
    CU_ASSERT(o->shape == _obl_at_fixed_address(OBL_STRING_SHAPE_ADDR));
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_string_size(o) == strlen(string));

    buffer = malloc(sizeof(char) * obl_string_size(o));
    CU_ASSERT(obl_string_chars(o, buffer, obl_string_size(o))
            == obl_string_size(o));
    CU_ASSERT(strncmp(buffer, string, obl_string_size(o)) == 0);

    CU_ASSERT(obl_string_ccmp(o, string) == 0);

    free(buffer);
    obl_destroy_object(o);
}

void test_fixed_object(void)
{
    const size_t length = 3;
    struct obl_object *items[length];
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *o;
    int i;

    d = obl_open_defdatabase(NULL);
    s = obl_create_session(d);

    o = obl_create_fixed(length);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->session == NULL);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->shape == obl_at_address(s, OBL_FIXED_SHAPE_ADDR));
    CU_ASSERT(obl_fixed_size(o) == length);
    CU_ASSERT(obl_fixed_at(o, 1) == obl_nil());

    items[0] = obl_create_integer((obl_int) 100);
    items[1] = obl_create_integer((obl_int) 101);
    items[2] = obl_create_integer((obl_int) 102);

    obl_fixed_at_put(o, 0, items[0]);
    obl_fixed_at_put(o, 1, items[1]);
    obl_fixed_at_put(o, 2, items[2]);

    CU_ASSERT(obl_fixed_at(o, 1) == items[1]);
    CU_ASSERT(obl_integer_value(obl_fixed_at(o, 2)) == 102);

    d->configuration.log_level = L_NONE;
    o->session = s;
    CU_ASSERT(obl_database_ok(d));
    CU_ASSERT(obl_fixed_at(o, 3) == obl_nil());
    CU_ASSERT(!obl_database_ok(d));
    obl_clear_error(d);

    for (i = 0; i < length; i++) {
        obl_destroy_object(items[i]);
    }

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_shape_object(void)
{
    char *slot_names[] = { "one", "two" };
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *o;
    struct obl_shape_storage *storage;

    d = obl_open_defdatabase(NULL);
    s = obl_create_session(d);

    o = obl_create_cshape("Foo", 2, slot_names, OBL_SLOTTED);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->session == NULL);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->shape == obl_nil());

    storage = o->storage.shape_storage;
    CU_ASSERT_FATAL(storage != NULL);
    CU_ASSERT(obl_fixed_size(storage->slot_names) == 2);
    CU_ASSERT(obl_string_ccmp(obl_fixed_at(storage->slot_names, 0), "one") == 0);
    CU_ASSERT(obl_string_ccmp(obl_fixed_at(storage->slot_names, 1), "two") == 0);

    CU_ASSERT(obl_shape_slotcount(o) == 2);
    CU_ASSERT(obl_shape_slotcnamed(o, "one") == 0);
    CU_ASSERT(obl_shape_slotcnamed(o, "two") == 1);
    CU_ASSERT(obl_shape_slotcnamed(o, "flabargh") == OBL_SENTINEL);
    CU_ASSERT(obl_shape_storagetype(o) == OBL_SLOTTED);

    obl_destroy_cshape(o);
    obl_close_database(d);
}

void test_slotted_object(void)
{
    char *slot_names[] = { "foo" , "bar" };
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *shape;
    struct obl_object *o;
    struct obl_object *value;

    d = obl_open_defdatabase(NULL);
    s = obl_create_session(d);

    shape = obl_create_cshape("FooClass", 2, slot_names, OBL_SLOTTED);
    o = obl_create_slotted(shape);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);

    CU_ASSERT(obl_database_ok(d));
    CU_ASSERT(obl_slotted_atcnamed(o, "foo") == obl_nil());
    CU_ASSERT(obl_slotted_atcnamed(o, "bar") == obl_nil());

    value = obl_create_integer((obl_int) 4);
    obl_slotted_atcnamed_put(o, "foo", value);
    CU_ASSERT(obl_slotted_atcnamed(o, "foo") == value);
    CU_ASSERT(obl_slotted_at(o, 0) == value);
    CU_ASSERT(obl_slotted_atcnamed(o, "bar") == obl_nil());
    CU_ASSERT(obl_slotted_at(o, 1) == obl_nil());

    d->configuration.log_level = L_NONE;
    o->session = s;
    CU_ASSERT(obl_database_ok(d));
    CU_ASSERT(obl_slotted_at(o, 7) == obl_nil());
    CU_ASSERT(!obl_database_ok(d));
    obl_clear_error(d);

    obl_destroy_cshape(shape);
    obl_destroy_object(o);
    obl_destroy_object(value);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_stub_object(void)
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *o;

    d = obl_open_defdatabase(NULL);
    s = obl_create_session(d);

    o = _obl_create_stub(s, (obl_logical_address) 14);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->logical_address == (obl_logical_address) 14)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_shape_storagetype(o->shape) == OBL_STUB);
    CU_ASSERT(_obl_is_stub(o));

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_boolean_object(void)
{
    CU_ASSERT(obl_boolean_value(obl_true()));
    CU_ASSERT(! obl_boolean_value(obl_false()));
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_object_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("object", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    ADD_TEST(test_integer_object);
    ADD_TEST(test_string_object);
    ADD_TEST(test_fixed_object);
    ADD_TEST(test_shape_object);
    ADD_TEST(test_slotted_object);
    ADD_TEST(test_boolean_object);

    return pSuite;
}
