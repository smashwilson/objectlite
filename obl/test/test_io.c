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
    o = obl_read_integer(shape, (obl_uint*) contents, 0);
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
    o = obl_read_string(shape, (obl_uint*) contents, 0);
    CU_ASSERT(obl_string_size(o) == 4);
    CU_ASSERT(obl_string_ccmp(o, "abcd") == 0);

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
                "test_mmap",
                test_mmap) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
