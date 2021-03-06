/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file test_io.c
 *
 * Unit tests for the object read and write primitives.  These functions provide
 * object serialization and deserialization directly to and from open files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CUnit/Basic.h"

#include "storage/object.h"
#include "database.h"
#include "platform.h"
#include "session.h"
#include "set.h"
#include "unitutilities.h"

static const char *filename = "testing.obl";

void test_read_integer(void)
{
    struct obl_object *shape, *o;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x11, 0x22, 0x33, 0x44); /* value */

    shape = obl_at_address(s, OBL_INTEGER_SHAPE_ADDR);
    o = obl_integer_read(s, shape, d->content,
            (obl_physical_address) 0, 0);
    CU_ASSERT(obl_integer_value(o) == 0x11223344);
    CU_ASSERT(o->physical_address == (obl_physical_address) 0);

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_string(void)
{
    struct obl_object *shape, *o;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0x04); /* length word */
    SET_CHAR(d->content, 2, 0x00,  'a', 0x00,  'b');
    SET_CHAR(d->content, 3, 0x00,  'c', 0x00,  'd');

    shape = obl_at_address(s, OBL_STRING_SHAPE_ADDR);
    o = obl_string_read(s, shape, d->content,
            (obl_physical_address) 0, 0);
    CU_ASSERT(obl_string_size(o) == 4);
    CU_ASSERT(obl_string_ccmp(o, "abcd") == 0);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_fixed(void)
{
    struct obl_object *shape, *o, *stub, *linked;
    struct obl_object *one, *two, *three, *four;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0x04); /* length */
    SET_CHAR(d->content, 2, 0x00, 0x00, 0x0A, 0x0B); /* slot 0 */
    SET_CHAR(d->content, 3, 0x00, 0x00, 0x0B, 0x0C); /* slot 1 */
    SET_CHAR(d->content, 4, 0x00, 0x00, 0x0C, 0x0D); /* slot 2 */
    SET_CHAR(d->content, 5, 0x00, 0x00, 0x0D, 0x0E); /* slot 3 */

    shape = obl_at_address(s, OBL_FIXED_SHAPE_ADDR);

    /*
     * With depth 0, obl_fixed_read should create STUB objects for all linked
     * addresses read.  obl_fixed_at() should resolve these stubs, so test
     * them by direct inspection.
     */
    o = obl_fixed_read(s, shape, d->content,
            (obl_physical_address) 0, 0);
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
     * With depth 1, obl_fixed_read should populate its members with actual
     * obl_objects acquired from obl_at_address.  Prepopulate the read set so
     * that those calls have something to find.
     */
    one = obl_create_integer((obl_int) 427);
    two = obl_create_cstring("foo", (obl_uint) 3);
    three = obl_create_integer((obl_int) 3442);
    four = obl_create_cstring("bar", (obl_uint) 3);

    one->logical_address = (obl_logical_address) 0x0A0B;
    two->logical_address = (obl_logical_address) 0x0B0C;
    three->logical_address = (obl_logical_address) 0x0C0D;
    four->logical_address = (obl_logical_address) 0x0D0E;

    obl_set_insert(s->read_set, one);
    obl_set_insert(s->read_set, two);
    obl_set_insert(s->read_set, three);
    obl_set_insert(s->read_set, four);

    o = obl_fixed_read(s, shape, d->content,
            (obl_physical_address) 0, 1);
    linked = o->storage.fixed_storage->contents[0];
    CU_ASSERT(linked == one);
    linked = o->storage.fixed_storage->contents[1];
    CU_ASSERT(linked == two);
    linked = o->storage.fixed_storage->contents[2];
    CU_ASSERT(linked == three);
    linked = o->storage.fixed_storage->contents[3];
    CU_ASSERT(linked == four);

    obl_destroy_object(o);
    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_shape(void)
{
    struct obl_object *name, *slot_names;
    struct obl_object *slot_one_name, *slot_two_name;
    struct obl_object *out;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0x01); /* name addr */
    SET_CHAR(d->content, 2, 0x00, 0x00, 0x00, 0x02); /* slot names addr */
    SET_UINT(d->content, 3, OBL_NIL_ADDR); /* current shape */
    SET_CHAR(d->content, 4, 0x00, 0x00, 0x00, 0x01); /* OBL_SLOTTED = format 1 */

    name = obl_create_cstring("FooClass", 8);
    name->logical_address = (obl_logical_address) 1;

    slot_one_name = obl_create_cstring("first slot", 10);
    slot_two_name = obl_create_cstring("second slot", 11);
    slot_names = obl_create_fixed((obl_uint) 2);
    obl_fixed_at_put(slot_names, 0, slot_one_name);
    obl_fixed_at_put(slot_names, 1, slot_two_name);
    slot_names->logical_address = (obl_logical_address) 2;

    obl_set_insert(s->read_set, name);
    obl_set_insert(s->read_set, slot_names);

    out = obl_shape_read(s, obl_nil(), d->content,
            (obl_physical_address) 0, 2);
    CU_ASSERT(obl_shape_storagetype(out) == OBL_SLOTTED);
    CU_ASSERT(out->storage.shape_storage->name == name);
    CU_ASSERT(out->storage.shape_storage->slot_names == slot_names);
    CU_ASSERT(out->storage.shape_storage->current_shape == obl_nil(d));

    obl_destroy_object(slot_one_name);
    obl_destroy_object(slot_two_name);
    obl_destroy_object(out);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_slotted(void)
{
    char *slot_names[2] = {
            "one", "two"
    };
    struct obl_object *shape;
    struct obl_object *one, *two;
    struct obl_object *o;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0xAA); /* slot 0 */
    SET_CHAR(d->content, 2, 0x00, 0x00, 0x00, 0xBB); /* slot 1 */

    shape = obl_create_cshape("FooClass", 2, slot_names, OBL_SLOTTED);
    one = obl_create_integer((obl_int) -17);
    one->logical_address = (obl_logical_address) 0xAA;
    two = obl_create_cstring("value", 5);
    two->logical_address = (obl_logical_address) 0xBB;

    obl_set_insert(s->read_set, one);
    obl_set_insert(s->read_set, two);

    o = obl_slotted_read(s, shape, d->content,
            (obl_physical_address) 0, 1);
    CU_ASSERT(obl_slotted_at(o, 0) == one);
    CU_ASSERT(obl_slotted_at(o, 1) == two);
    CU_ASSERT(obl_slotted_atcnamed(o, "one") == one);
    CU_ASSERT(obl_slotted_atcnamed(o, "two") == two);

    obl_destroy_object(o);
    obl_destroy_cshape(shape);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_addrtreepage(void)
{
    struct obl_object *treepage, *shape;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0x02); /* depth */
    SET_CHAR(d->content, 3, 0x01, 0x02, 0x03, 0x04); /* 0x01 = next tree page */

    shape = obl_at_address(s, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    treepage = obl_addrtreepage_read(s, shape, d->content, 0, 1);

    CU_ASSERT(treepage->storage.addrtreepage_storage->height == (obl_uint) 2);
    CU_ASSERT(treepage->storage.addrtreepage_storage->contents[0] ==
            OBL_PHYSICAL_UNASSIGNED);
    CU_ASSERT(treepage->storage.addrtreepage_storage->contents[1] ==
            (obl_physical_address) 0x01020304);

    obl_destroy_object(treepage);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_read_arbitrary(void)
{
    struct obl_object *integer, *string;
    struct obl_database *d = obl_open_defdatabase(NULL);
    struct obl_session *s = obl_create_session(d);

    wipe(d);
    SET_UINT(d->content, 0, OBL_INTEGER_SHAPE_ADDR); /* Shape word */
    SET_CHAR(d->content, 1, 0x00, 0x00, 0x00, 0x0A); /* Integer value */
    SET_UINT(d->content, 2, OBL_STRING_SHAPE_ADDR);  /* Shape word */
    SET_CHAR(d->content, 3, 0x00, 0x00, 0x00, 0x05); /* Length: 5*/
    SET_CHAR(d->content, 4, 0x00, 0x68, 0x00, 0x65); /* 'h' 'e' */
    SET_CHAR(d->content, 5, 0x00, 0x6C, 0x00, 0x6C); /* 'l' 'l' */
    SET_CHAR(d->content, 6, 0x00, 0x6F, 0x00, 0x00); /* 'o' (pad) */

    integer = obl_read_object(s, d->content, 0, 1);
    CU_ASSERT(integer->shape == obl_at_address(s, OBL_INTEGER_SHAPE_ADDR));
    CU_ASSERT(obl_integer_value(integer) == (obl_int) 10);

    string = obl_read_object(s, d->content, 2, 1);
    CU_ASSERT(string->shape == obl_at_address(s, OBL_STRING_SHAPE_ADDR));
    CU_ASSERT(obl_string_ccmp(string, "hello") == 0);

    obl_destroy_object(integer);
    obl_destroy_object(string);

    obl_destroy_session(s);
    obl_close_database(d);
}

void test_write_integer(void)
{
    struct obl_object *o;
    const char expected[8] = { 0 };
    struct obl_database *d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x12, 0x34, 0x56, 0x78);

    o = obl_create_integer((obl_int) 0x12345678);
    o->physical_address = (obl_physical_address) 0;
    obl_integer_write(o, d->content);

    CU_ASSERT(memcmp(d->content, expected, 8) == 0);

    obl_destroy_object(o);
    obl_close_database(d);
}

void test_write_string(void)
{
    struct obl_database *d;
    struct obl_object *o;
    const char expected[20] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x00, 0x00, 0x00, 0x05); /* length word */
    SET_CHAR(expected, 2, 0x00, 0x68, 0x00, 0x65); /* 'h' 'e' */
    SET_CHAR(expected, 3, 0x00, 0x6C, 0x00, 0x6C); /* 'l' 'l' */
    SET_CHAR(expected, 4, 0x00, 0x6F, 0x00, 0x00); /* 'o' pad */

    o = obl_create_cstring("hello", 5);
    o->physical_address = (obl_physical_address) 0;
    obl_string_write(o, d->content);

    CU_ASSERT(memcmp(d->content, expected, 20) == 0);

    obl_destroy_object(o);
    obl_close_database(d);
}

void test_write_fixed(void)
{
    struct obl_database *d;
    struct obl_object *o;
    struct obl_object *one, *two, *three;
    const char expected[20] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x00, 0x00, 0x00, 0x03); /* Length word. */
    SET_CHAR(expected, 2, 0x00, 0x00, 0x00, 0xAA); /* Object one. */
    SET_CHAR(expected, 3, 0x00, 0x00, 0x00, 0xBB); /* Object two. */
    SET_CHAR(expected, 4, 0x00, 0x00, 0x00, 0xCC); /* Object three. */

    one = obl_create_integer((obl_int) 4123);
    one->logical_address = (obl_logical_address) 0x00AA;
    two = obl_create_integer((obl_int) 1002);
    two->logical_address = (obl_logical_address) 0x00BB;
    three = obl_create_integer((obl_int) 37);
    three->logical_address = (obl_logical_address) 0x00CC;

    o = obl_create_fixed((obl_uint) 3);
    o->physical_address = (obl_physical_address) 0;
    obl_fixed_at_put(o, 0, one);
    obl_fixed_at_put(o, 1, two);
    obl_fixed_at_put(o, 2, three);

    obl_fixed_write(o, d->content);

    CU_ASSERT(memcmp(d->content, expected, 20) == 0);

    obl_destroy_object(o);
    obl_destroy_object(one);
    obl_destroy_object(two);
    obl_destroy_object(three);
    obl_close_database(d);
}

void test_write_shape(void)
{
    struct obl_database *d;
    struct obl_object *shape;
    char *slot_names[] = { "first", "second" };
    const char expected[20] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x00, 0x00, 0xAA, 0xBB); /* Name address */
    SET_CHAR(expected, 2, 0x00, 0x00, 0xCC, 0xDD); /* Slot names address */
    SET_CHAR(expected, 3, 0xff, 0xff, 0xff, 0xf1); /* Current shape = OBL_NIL_ADDR */
    SET_CHAR(expected, 4, 0x00, 0x00, 0x00, 0x01);  /* Storage type = OBL_SLOTTED */

    shape = obl_create_cshape("FooClass", 2, slot_names, OBL_SLOTTED);
    shape->physical_address = (obl_physical_address) 0;

    shape->storage.shape_storage->name->logical_address =
            (obl_logical_address) 0xAABB;
    shape->storage.shape_storage->slot_names->logical_address =
            (obl_logical_address) 0xCCDD;

    obl_shape_write(shape, d->content);
    CU_ASSERT(memcmp(d->content, expected, 20) == 0);

    obl_destroy_cshape(shape);
    obl_close_database(d);
}

void test_write_slotted(void)
{
    struct obl_database *d;
    struct obl_object *shape, *slotted;
    struct obl_object *aaa, *bbb, *ccc;
    char *slot_names[] = { "aaa", "bbb", "ccc" };
    const char expected[16] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x00, 0x00, 0x11, 0xAA); /* Slot "aaa" address. */
    SET_CHAR(expected, 2, 0x00, 0x00, 0x22, 0xBB); /* Slot "bbb" address. */
    SET_CHAR(expected, 3, 0x00, 0x00, 0x33, 0xCC); /* Slot "ccc" address. */

    shape = obl_create_cshape("FooClass", 3, slot_names, OBL_SLOTTED);
    shape->physical_address = (obl_physical_address) 0;

    slotted = obl_create_slotted(shape);

    aaa = obl_create_integer((obl_int) 1);
    aaa->logical_address = (obl_logical_address) 0x11AA;
    bbb = obl_create_integer((obl_int) 2);
    bbb->logical_address = (obl_logical_address) 0x22BB;
    ccc = obl_create_integer((obl_int) 3);
    ccc->logical_address = (obl_logical_address) 0x33CC;

    obl_slotted_at_put(slotted, 0, aaa);
    obl_slotted_at_put(slotted, 1, bbb);
    obl_slotted_at_put(slotted, 2, ccc);

    obl_slotted_write(slotted, d->content);
    CU_ASSERT(memcmp(d->content, expected, 16) == 0);

    obl_destroy_object(aaa);
    obl_destroy_object(bbb);
    obl_destroy_object(ccc);
    obl_destroy_object(slotted);
    obl_destroy_cshape(shape);
    obl_close_database(d);
}

void test_write_addrtreepage(void)
{
    struct obl_database *d;
    struct obl_object *treepage;
    const char expected[4 + CHUNK_SIZE * 4] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_CHAR(expected, 1, 0x00, 0x00, 0x00, 0x04); /* Page height. */
    SET_CHAR(expected, 3, 0x00, 0xAA, 0x00, 0xBB); /* 0x01 = next treepage */

    treepage = obl_create_addrtreepage((obl_uint) 4);
    treepage->storage.addrtreepage_storage->contents[1] =
            (obl_physical_address) 0x00AA00BB;

    obl_addrtreepage_write(treepage, d->content);
    CU_ASSERT(memcmp(d->content, expected, 4 + CHUNK_SIZE * 4) == 0);

    obl_destroy_object(treepage);
    obl_close_database(d);
}

void test_write_arbitrary(void)
{
    struct obl_database *d;
    struct obl_object *one, *two;
    char expected[28] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    /* Physical 0: the string 'hello' */
    SET_UINT(expected, 0, OBL_STRING_SHAPE_ADDR); /* String shape */
    SET_CHAR(expected, 1, 0x00, 0x00, 0x00, 0x05); /* Length: 5 */
    SET_CHAR(expected, 2, 0x00, 0x68, 0x00, 0x65); /* 'h' 'e' */
    SET_CHAR(expected, 3, 0x00, 0x6C, 0x00, 0x6C); /* 'l' 'l' */
    SET_CHAR(expected, 4, 0x00, 0x6F, 0x00, 0x00); /* 'o' padding byte */

    /* Physical 5: the integer '42' */
    SET_UINT(expected, 5, OBL_INTEGER_SHAPE_ADDR); /* Integer shape */
    SET_CHAR(expected, 6, 0x00, 0x00, 0x00, 0x2A); /* Integer value */

    one = obl_create_cstring("hello", 5);
    one->physical_address = (obl_physical_address) 0;
    two = obl_create_integer((obl_int) 42);
    two->physical_address = (obl_physical_address) 5;

    obl_write_object(one, d->content);
    obl_write_object(two, d->content);

    CU_ASSERT(memcmp(d->content, expected, 28) == 0);

    obl_destroy_object(one);
    obl_destroy_object(two);
    obl_close_database(d);
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

    f = fopen(filename, "wb");
    for (i = 0; i < 10; i++) {
        fputc(contents[i], f);
    }
    fclose(f);

    f = fopen(filename, "r+b");
    fd = fileno(f);

    mapped = (char*) mmap(0, 10, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
    f = fopen(filename, "rb");
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

    ADD_TEST(test_read_integer);
    ADD_TEST(test_read_string);
    ADD_TEST(test_read_fixed);
    ADD_TEST(test_read_shape);
    ADD_TEST(test_read_slotted);
    ADD_TEST(test_read_addrtreepage);
    ADD_TEST(test_read_arbitrary);
    ADD_TEST(test_write_integer);
    ADD_TEST(test_write_string);
    ADD_TEST(test_write_fixed);
    ADD_TEST(test_write_shape);
    ADD_TEST(test_write_slotted);
    ADD_TEST(test_write_addrtreepage);
    ADD_TEST(test_write_arbitrary);
    ADD_TEST(test_mmap);

    return pSuite;
}
