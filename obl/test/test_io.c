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

#include "platform.h"
#include "io.h"
#include "object.h"

#define FILENAME "testing.obl"

void test_read_integer(void)
{
    CU_FAIL("I/O tests disabled for the moment.");

    /*
    FILE *writable, *readable;
    obl_integer_storage payload = 0x11223344;
    uint32_t netlong;
    struct obl_object shape;
    struct obl_object *output;

    writable = fopen(FILENAME, "wb");
    if (writable == NULL) {
        CU_FAIL("Unable to create temporary data file.");
        return;
    }
    netlong = htonl((uint32_t) payload);
    CU_ASSERT(fwrite(&netlong, sizeof(uint32_t), 1, writable) == 1);
    CU_ASSERT_FATAL(fclose(writable) == 0);

    readable = fopen(FILENAME, "rb");
    if (readable == NULL) {
        CU_FAIL("Unable to reopen temporary data file.");
        return;
    }

    shape.storage.shape_storage = (obl_shape_storage*) malloc(
            sizeof(obl_shape_storage));
    shape.storage.shape_storage->storage_format = OBL_INTEGER;

    output = obl_read_integer(&shape, readable);
    CU_ASSERT(fclose(readable) == 0);
    CU_ASSERT(output != NULL);

    CU_ASSERT(output->shape == &shape);
    CU_ASSERT(*(output->storage.integer_storage) == 0x11223344);

    free(output);
    */
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
                "Read an Integer object from raw bytes.",
                test_read_integer) == NULL) ||
        (CU_add_test(pSuite,
                "Verify that mmap() is functional.",
                test_mmap) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
