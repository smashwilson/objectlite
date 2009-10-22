/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for struct obl_object creation, access, and manipulation API.
 */

#include "storage/object.h"

#include <string.h>

#include "CUnit/Headers/Basic.h"
#include "unicode/ucnv.h"

#include "database.h"
#include "log.h"
#include "unitutilities.h"

void test_create_integer(void)
{
    struct obl_database *d;
    struct obl_object *o;

    d = obl_create_database("unit.obl");

    o = obl_create_integer(d, (obl_int) 42);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_INTEGER_SHAPE_ADDR));
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_integer_value(o) == (obl_int) 42);

    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_create_string(void)
{
    char *string = "NULL-terminated C string.";
    struct obl_database *d;
    struct obl_object *o;
    char *buffer;
    int i;

    d = obl_create_database("unit.obl");

    o = obl_create_cstring(d, string, strlen(string));
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_STRING_SHAPE_ADDR));
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_string_size(o) == strlen(string));

    buffer = (char *) malloc(sizeof(char) * obl_string_size(o));
    CU_ASSERT(obl_string_chars(o, buffer, obl_string_size(o))
            == obl_string_size(o));
    CU_ASSERT(strncmp(buffer, string, obl_string_size(o)) == 0);

    CU_ASSERT(obl_string_ccmp(o, string) == 0);

    free(buffer);
    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_create_fixed(void)
{
    const size_t length = 3;
    struct obl_object *items[length];
    struct obl_database *d;
    struct obl_object *o;
    int i;

    d = obl_create_database("unit.obl");

    o = obl_create_fixed(d, length);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_FIXED_SHAPE_ADDR));
    CU_ASSERT(obl_fixed_size(o) == length);
    CU_ASSERT(obl_fixed_at(o, 1) == obl_nil(d));

    items[0] = obl_create_integer(d, (obl_int) 100);
    items[1] = obl_create_integer(d, (obl_int) 101);
    items[2] = obl_create_integer(d, (obl_int) 102);

    obl_fixed_at_put(o, 0, items[0]);
    obl_fixed_at_put(o, 1, items[1]);
    obl_fixed_at_put(o, 2, items[2]);

    CU_ASSERT(obl_fixed_at(o, 1) == items[1]);
    CU_ASSERT(obl_integer_value(obl_fixed_at(o, 2)) == 102);

    for (i = 0; i < length; i++) {
        obl_destroy_object(items[i]);
    }
    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_create_shape(void)
{
    char *slot_names[] = { "one", "two" };
    struct obl_database *d;
    struct obl_object *o;
    struct obl_shape_storage *storage;

    d = obl_create_database("unit.obl");

    o = obl_create_cshape(d, "Foo", 2, slot_names, OBL_SLOTTED);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(o->shape == obl_nil(d));

    storage = o->storage.shape_storage;
    CU_ASSERT_FATAL(storage != NULL);
    CU_ASSERT(obl_fixed_size(storage->slot_names) == 2);
    CU_ASSERT(obl_string_ccmp(obl_fixed_at(storage->slot_names, 0), "one") == 0);
    CU_ASSERT(obl_string_ccmp(obl_fixed_at(storage->slot_names, 1), "two") == 0);

    CU_ASSERT(obl_shape_slotcount(o) == 2);
    CU_ASSERT(obl_shape_slotcnamed(o, "one") == 0);
    CU_ASSERT(obl_shape_slotcnamed(o, "two") == 1);
    CU_ASSERT(obl_shape_storagetype(o) == OBL_SLOTTED);

    obl_destroy_cshape(o);
    obl_destroy_database(d);
}

void test_create_slotted(void)
{
    char *slot_names[] = { "foo" , "bar" };
    struct obl_database *d;
    struct obl_object *shape;
    struct obl_object *o;
    struct obl_object *value;

    d = obl_create_database("unit.obl");

    shape = obl_create_cshape(d, "FooClass", 2, slot_names, OBL_SLOTTED);
    o = obl_create_slotted(shape);
    CU_ASSERT(o->logical_address == OBL_LOGICAL_UNASSIGNED)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);

    CU_ASSERT(obl_database_ok(d));
    CU_ASSERT(obl_slotted_atcnamed(o, "foo") == obl_nil(d));
    CU_ASSERT(obl_slotted_atcnamed(o, "bar") == obl_nil(d));

    value = obl_create_integer(d, (obl_int) 4);
    obl_slotted_atcnamed_put(o, "foo", value);
    CU_ASSERT(obl_slotted_atcnamed(o, "foo") == value);
    CU_ASSERT(obl_slotted_at(o, 0) == value);
    CU_ASSERT(obl_slotted_atcnamed(o, "bar") == obl_nil(d));
    CU_ASSERT(obl_slotted_at(o, 1) == obl_nil(d));

    obl_destroy_cshape(shape);
    obl_destroy_object(o);
    obl_destroy_object(value);
    obl_destroy_database(d);
}

void test_create_stub(void)
{
    struct obl_database *d;
    struct obl_object *o, *real;

    d = obl_create_database("unit.obl");

    o = _obl_create_stub(d, (obl_logical_address) 14);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->logical_address == (obl_logical_address) 14)
    CU_ASSERT(o->physical_address == OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(obl_shape_storagetype(o->shape) == OBL_STUB);
    CU_ASSERT(_obl_is_stub(o));

    real = obl_create_integer(d, (obl_int) 42);
    real->logical_address = (obl_logical_address) 14;
    obl_cache_insert(d->cache, real);

    CU_ASSERT(_obl_resolve_stub(o, 0) == real);

    obl_destroy_object(real);
    obl_destroy_database(d);
}

void test_boolean_objects(void)
{
    struct obl_database *d;

    d = obl_create_database("unit.obl");

    CU_ASSERT(obl_boolean_value(obl_true(d)));
    CU_ASSERT(! obl_boolean_value(obl_false(d)));

    obl_destroy_database(d);
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

    ADD_TEST(test_create_integer);
    ADD_TEST(test_create_string);
    ADD_TEST(test_create_fixed);
    ADD_TEST(test_create_shape);
    ADD_TEST(test_create_slotted);
    ADD_TEST(test_boolean_objects);

    return pSuite;
}
