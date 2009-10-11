/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the functions contained in "io.c".  These functions provide
 * primitive object serialization and deserialization directly to and from
 * open files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CUnit/Headers/Basic.h"

#include "database.h"
#include "io.h"
#include "object.h"
#include "platform.h"

#define FILENAME "testing.obl"

void test_read_integer(void)
{
    /* Emulate big-endian (network) byte order storage. */
    char contents[4] = {
            0x11, 0x22, 0x33, 0x44
    };
    struct obl_database *d;
    struct obl_object *shape, *o;

    d = obl_create_database(FILENAME);

    shape = obl_at_address(d, OBL_INTEGER_SHAPE_ADDR);
    o = obl_read_integer(shape, (obl_uint*) contents, 0, 0);
    CU_ASSERT(obl_integer_value(o) == 0x11223344);
    CU_ASSERT(o->physical_address == (obl_physical_address) 0);

    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_read_string(void)
{
    /* length obl_uword, UTF-16BE */
    char contents[] = {
            0x0, 0x0, 0x0, 0x4,
            0x0, 'a',
            0x0, 'b',
            0x0, 'c',
            0x0, 'd'
    };
    struct obl_database *d;
    struct obl_object *shape, *o;

    d = obl_create_database(FILENAME);

    shape = obl_at_address(d, OBL_STRING_SHAPE_ADDR);
    o = obl_read_string(shape, (obl_uint*) contents, 0, 0);
    CU_ASSERT(obl_string_size(o) == 4);
    CU_ASSERT(obl_string_ccmp(o, "abcd") == 0);

    obl_destroy_database(d);
}

void test_read_fixed(void)
{
    /* length obl_uword, that many obl_logical_addresses */
    char contents[] = {
            0x00, 0x00, 0x00, 0x04,
            0x00, 0x00, 0x0A, 0x0B,
            0x00, 0x00, 0x0B, 0x0C,
            0x00, 0x00, 0x0C, 0x0D,
            0x00, 0x00, 0x0D, 0x0E
    };
    struct obl_database *d;
    struct obl_object *shape, *o, *stub, *linked;
    struct obl_object *one, *two, *three, *four;

    d = obl_create_database(FILENAME);
    shape = obl_at_address(d, OBL_FIXED_SHAPE_ADDR);

    /*
     * With depth 0, obl_read_fixed should create STUB objects for all linked
     * addresses read.  obl_fixed_at() should resolve these stubs, so test
     * them by direct inspection.
     */
    o = obl_read_fixed(shape, (obl_uint*) contents, 0, 0);
    CU_ASSERT(obl_fixed_size(o) == 4);
    stub = o->storage.fixed_storage->contents[0];
    CU_ASSERT(obl_shape_storagetype(stub->shape) == OBL_STUB);
    CU_ASSERT(stub->storage.stub_storage->value ==
            (obl_logical_address) 0x0A0B);
    stub = o->storage.fixed_storage->contents[3];
    CU_ASSERT(obl_shape_storagetype(stub->shape) == OBL_STUB);
    CU_ASSERT(stub->storage.stub_storage->value ==
            (obl_logical_address) 0x0D0E);
    obl_destroy_object(o);

    /*
     * With depth 1, obl_read_fixed should populate its members with actual
     * obl_objects acquired from obl_at_address.  Prepopulate the cache so
     * that those calls have something to find.
     */
    one = obl_create_integer(d, (obl_int) 427);
    two = obl_create_cstring(d, "foo", (obl_uint) 3);
    three = obl_create_integer(d, (obl_int) 3442);
    four = obl_create_cstring(d, "bar", (obl_uint) 3);

    one->logical_address = (obl_logical_address) 0x0A0B;
    two->logical_address = (obl_logical_address) 0x0B0C;
    three->logical_address = (obl_logical_address) 0x0C0D;
    four->logical_address = (obl_logical_address) 0x0D0E;

    obl_cache_insert(d->cache, one);
    obl_cache_insert(d->cache, two);
    obl_cache_insert(d->cache, three);
    obl_cache_insert(d->cache, four);

    o = obl_read_fixed(shape, (obl_uint*) contents, 0, 1);
    linked = o->storage.fixed_storage->contents[0];
    CU_ASSERT(linked == one);
    linked = o->storage.fixed_storage->contents[1];
    CU_ASSERT(linked == two);
    linked = o->storage.fixed_storage->contents[2];
    CU_ASSERT(linked == three);
    linked = o->storage.fixed_storage->contents[3];
    CU_ASSERT(linked == four);

    obl_destroy_object(o);
    obl_destroy_object(one);
    obl_destroy_object(two);
    obl_destroy_object(three);
    obl_destroy_object(four);

    obl_destroy_database(d);
}

void test_read_shape(void)
{
    /*
     * logical addresses for: name, slot names, current shape; obl_uint for
     * storage format.
     */
    char contents[] = {
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x02,
            0xff, 0xff, 0xff, 0xf3, /* OBL_NIL_ADDR */
            0x00, 0x00, 0x00, 0x01, /* OBL_SLOTTED = format 2 */
    };
    struct obl_database *d;
    struct obl_object *name, *slot_names;
    struct obl_object *slot_one_name, *slot_two_name;
    struct obl_object *out;

    d = obl_create_database(FILENAME);

    name = obl_create_cstring(d, "FooClass", 8);
    name->logical_address = (obl_logical_address) 1;

    slot_one_name = obl_create_cstring(d, "first slot", 10);
    slot_two_name = obl_create_cstring(d, "second slot", 11);
    slot_names = obl_create_fixed(d, (obl_uint) 2);
    obl_fixed_at_put(slot_names, 0, slot_one_name);
    obl_fixed_at_put(slot_names, 1, slot_two_name);
    slot_names->logical_address = (obl_logical_address) 2;

    obl_cache_insert(d->cache, name);
    obl_cache_insert(d->cache, slot_names);

    out = obl_read_shape(obl_nil(d), (obl_uint*) contents, 0, 2);
    CU_ASSERT(obl_shape_storagetype(out) == OBL_SLOTTED);
    CU_ASSERT(out->storage.shape_storage->name == name);
    CU_ASSERT(out->storage.shape_storage->slot_names == slot_names);
    CU_ASSERT(out->storage.shape_storage->current_shape == obl_nil(d));

    obl_destroy_object(name);
    obl_destroy_object(slot_one_name);
    obl_destroy_object(slot_two_name);
    obl_destroy_object(slot_names);
    obl_destroy_object(out);

    obl_destroy_database(d);
}

void test_read_slotted(void)
{
    char contents[] = {
            0x00, 0x00, 0x00, 0xAA,
            0x00, 0x00, 0x00, 0xBB
    };
    char *slot_names[2] = {
            "one", "two"
    };
    struct obl_database *d;
    struct obl_object *shape;
    struct obl_object *one, *two;
    struct obl_object *o;

    d = obl_create_database(FILENAME);

    shape = obl_create_cshape(d, "FooClass", 2, slot_names, OBL_SLOTTED);
    one = obl_create_integer(d, (obl_int) -17);
    one->logical_address = (obl_logical_address) 0xAA;
    two = obl_create_cstring(d, "value", 5);
    two->logical_address = (obl_logical_address) 0xBB;

    obl_cache_insert(d->cache, one);
    obl_cache_insert(d->cache, two);

    o = obl_read_slotted(shape, (obl_uint *) contents, 0, 1);
    CU_ASSERT(obl_slotted_at(o, 0) == one);
    CU_ASSERT(obl_slotted_at(o, 1) == two);
    CU_ASSERT(obl_slotted_atcnamed(o, "one") == one);
    CU_ASSERT(obl_slotted_atcnamed(o, "two") == two);

    obl_destroy_object(o);
    obl_destroy_object(one);
    obl_destroy_object(two);
    obl_destroy_cshape(shape);

    obl_destroy_database(d);
}

void test_read_arbitrary(void)
{
    char contents[] = {
            /* Physical 0: an integer */
            0xff, 0xff, 0xff, 0xf6, /* Integer shape addr */
            0x00, 0x00, 0x00, 0x0A, /* Integer value */
            /* Physical 2: a string */
            0xff, 0xff, 0xff, 0xfa, /* String shape addr */
            0x00, 0x00, 0x00, 0x05, /* Length: 5 */
            0x00, 0x68, 0x00, 0x65, /* 'h' 'e' */
            0x00, 0x6C, 0x00, 0x6C, /* 'l' 'l' */
            0x00, 0x6F, 0x00, 0x00, /* 'o' padding byte */
    };
    struct obl_database *d;
    struct obl_object *integer, *string;

    d = obl_create_database(FILENAME);

    integer = obl_read_object(d, (obl_uint*) contents, 0, 1);
    CU_ASSERT(integer->shape == obl_at_address(d, OBL_INTEGER_SHAPE_ADDR));
    CU_ASSERT(obl_integer_value(integer) == (obl_int) 10)

    string = obl_read_object(d, (obl_uint*) contents, 2, 1);
    CU_ASSERT(string->shape == obl_at_address(d, OBL_STRING_SHAPE_ADDR));
    CU_ASSERT(obl_string_ccmp(string, "hello") == 0);

    obl_destroy_object(integer);
    obl_destroy_object(string);

    obl_destroy_database(d);
}

void test_write_integer(void)
{
    struct obl_database *d;
    struct obl_object *o;
    char contents[8] = { 0 };
    const char expected[] = {
            0x00, 0x00, 0x00, 0x00, /* Space for the shape address. */
            0x12, 0x34, 0x56, 0x78
    };

    d = obl_create_database(FILENAME);

    o = obl_create_integer(d, (obl_int) 0x12345678);
    o->physical_address = (obl_physical_address) 0;
    obl_write_integer(o, (obl_uint*) contents);

    CU_ASSERT(memcmp(contents, expected, 8) == 0);

    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_write_string(void)
{
    struct obl_database *d;
    struct obl_object *o;
    char contents[20] = { 0 };
    const char expected[20] = {
            0x00, 0x00, 0x00, 0x00, /* Space for the shape address. */
            0x00, 0x00, 0x00, 0x05, /* Length word. */
            0x00, 0x68, 0x00, 0x65, /* 'h' 'e' */
            0x00, 0x6C, 0x00, 0x6C, /* 'l' 'l' */
            0x00, 0x6F, 0x00, 0x00  /* 'o' padding */
    };

    d = obl_create_database(FILENAME);

    o = obl_create_cstring(d, "hello", 5);
    o->physical_address = (obl_physical_address) 0;
    obl_write_string(o, (obl_uint*) contents);

    CU_ASSERT(memcmp(contents, expected, 20) == 0);

    obl_destroy_object(o);
    obl_destroy_database(d);
}

void test_write_fixed(void)
{
    struct obl_database *d;
    struct obl_object *o;
    struct obl_object *one, *two, *three;
    char contents[20] = { 0 };
    const char expected[20] = {
            0x00, 0x00, 0x00, 0x00, /* Space for the shape address. */
            0x00, 0x00, 0x00, 0x03, /* Length word. */
            0x00, 0x00, 0x00, 0xAA, /* Object one. */
            0x00, 0x00, 0x00, 0xBB, /* Object two. */
            0x00, 0x00, 0x00, 0xCC, /* Object three. */
    };
    int i;

    d = obl_create_database(FILENAME);

    one = obl_create_integer(d, (obl_int) 4123);
    one->logical_address = (obl_logical_address) 0x00AA;
    two = obl_create_integer(d, (obl_int) 1002);
    two->logical_address = (obl_logical_address) 0x00BB;
    three = obl_create_integer(d, (obl_int) 37);
    three->logical_address = (obl_logical_address) 0x00CC;

    o = obl_create_fixed(d, (obl_uint) 3);
    o->physical_address = (obl_physical_address) 0;
    obl_fixed_at_put(o, 0, one);
    obl_fixed_at_put(o, 1, two);
    obl_fixed_at_put(o, 2, three);

    obl_write_fixed(o, (obl_uint*) contents);

    CU_ASSERT(memcmp(contents, expected, 20) == 0);

    obl_destroy_object(o);
    obl_destroy_object(one);
    obl_destroy_object(two);
    obl_destroy_object(three);
    obl_destroy_database(d);
}

/*
 * Verify that the (possibly emulated) version of mmap() currently being used
 * via platform.h actually works.
 */
void test_mmap(void)
{
    const char contents[10] = {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'
    };
    FILE *f;
    char *mapped;
    int i, fd;

    f = fopen(FILENAME, "wb");
    for (i = 0; i < 10; i++) {
        fputc(contents[i], f);
    }
    fclose(f);

    f = fopen(FILENAME, "r+b");
    fd = fileno(f);

    mapped = (char*) mmap(0, 10, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        CU_FAIL("Unable to map memory.");
        fclose(f);
        return ;
    }
    fclose(f);

    /* Ensure that data can be read properly from the mapped segment. */
    CU_ASSERT(mapped[0] == 'a');
    CU_ASSERT(mapped[4] == 'e');

    /* Write data back to the file via the mapped segment. */
    mapped[2] = 'z';

    munmap(mapped, 10);

    /* Check the written data in the actual file. */
    f = fopen(FILENAME, "rb");
    for (i = 0; i < 10; i++) {
        if (i == 2) {
            CU_ASSERT(fgetc(f) == 'z');
        } else {
            CU_ASSERT(fgetc(f) == contents[i]);
        }
    }
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_io_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("io", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    if (
        (CU_add_test(pSuite,
                "test_read_integer",
                test_read_integer) == NULL) ||
        (CU_add_test(pSuite,
                "test_read_string",
                test_read_string) == NULL) ||
        (CU_add_test(pSuite,
                "test_read_fixed",
                test_read_fixed) == NULL) ||
        (CU_add_test(pSuite,
                "test_read_shape",
                test_read_shape) == NULL) ||
        (CU_add_test(pSuite,
                "test_read_slotted",
                test_read_slotted) == NULL) ||
        (CU_add_test(pSuite,
                "test_read_arbitrary",
                test_read_arbitrary) == NULL) ||
        (CU_add_test(pSuite,
                "test_write_integer",
                test_write_integer) == NULL) ||
        (CU_add_test(pSuite,
                "test_write_string",
                test_write_string) == NULL) ||
        (CU_add_test(pSuite,
                "test_write_fixed",
                test_write_fixed) == NULL) ||
        (CU_add_test(pSuite,
                "test_mmap",
                test_mmap) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
