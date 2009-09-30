/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for obl_object creation, access, and manipulation API.
 */

#include "object.h"

#include <string.h>

#include "CUnit/Headers/Basic.h"
#include "unicode/ucnv.h"

#include "database.h"
#include "log.h"

void test_create_integer(void)
{
    obl_database *d;
    obl_object *o;

    d = obl_create_database("unit.obl");

    o = obl_create_integer(d, 42);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_INTEGER_SHAPE_ADDR));
    CU_ASSERT(obl_integer_value(o) == 42);

    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_create_string(void)
{
    char *string = "NULL-terminated C string.";
    obl_database *d;
    obl_object *o;
    char *buffer;
    int i;

    d = obl_create_database("unit.obl");

    o = obl_create_cstring(d, string, strlen(string));
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_STRING_SHAPE_ADDR));
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
    obl_object *items[length];
    obl_database *d;
    obl_object *o;
    int i;

    d = obl_create_database("unit.obl");

    o = obl_create_fixed(d, length);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == obl_at_address(d, OBL_FIXED_SHAPE_ADDR));
    CU_ASSERT(obl_fixed_size(o) == length);
    CU_ASSERT(obl_fixed_at(o, 1) == obl_at_address(d, OBL_NIL_ADDR));

    items[0] = obl_create_integer(d, 100);
    items[1] = obl_create_integer(d, 101);
    items[2] = obl_create_integer(d, 102);

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
    obl_database *d;
    obl_object *o;
    obl_shape_storage *storage;

    d = obl_create_database("unit.obl");

    o = obl_create_cshape(d, "Foo", 2, slot_names, OBL_SLOTTED);
    CU_ASSERT_FATAL(o != NULL);
    CU_ASSERT(o->database == d);
    CU_ASSERT(o->shape == NULL);

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

    if (
        (CU_add_test(pSuite,
                "Create an OBL_INTEGER object.",
                test_create_integer) == NULL) ||
        (CU_add_test(pSuite,
                "Create an OBL_STRING object from a C string.",
                test_create_string) == NULL) ||
        (CU_add_test(pSuite,
                "Create an OBL_FIXED collection.",
                test_create_fixed) == NULL) ||
        (CU_add_test(pSuite,
                "Create a shape object with convenience methods.",
                test_create_shape) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
