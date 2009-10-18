/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the logical to physical address mapping.
 */

#include "CUnit/Headers/Basic.h"

#include "addressmap.h"

#include "object.h"
#include "database.h"
#include "unitutilities.h"

const static char *filename = "addrmap.obl";

void test_map_leaf(void)
{
    struct obl_database *d;
    struct obl_object *addrtreepage;
    obl_physical_address result;
    const char contents[] = {
            0x00, 0x00, 0x00, 0x00, /* padding word */
            /* Physical address 1 */
            0xff, 0xff, 0xff, 0xfb, /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00, /* height = 0 */
            0x00, 0x00, 0x00, 0x00, /* 0x00 */
            0x00, 0x00, 0x1A, 0x2B, /* 0x01 */
    };

    d = obl_create_database(filename);
    d->content = (obl_uint*) contents;
    d->root.address_map_addr = (obl_physical_address) 1;

    result = obl_address_lookup(d, (obl_logical_address) 1);
    CU_ASSERT(result == (obl_physical_address) 0x1A2B);

    result = obl_address_lookup(d, (obl_logical_address) 0x400);
    CU_ASSERT(result == OBL_PHYSICAL_UNASSIGNED);

    obl_destroy_database(d);
}

void test_map_branch(void)
{
    struct obl_database *d;
    struct obl_object *branch, *leaf;
    obl_physical_address result;
    const char contents[] = {
            0x00, 0x00, 0x00, 0x00,  /* A padding word */
            /* Physical address 1 */
            0xff, 0xff, 0xff, 0xfb,  /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00,  /* height = 0 */
            0x00, 0x00, 0x00, 0x00,  /* 0x00 = OBL_PHYSICAL_UNASSIGNED */
            0x00, 0xAA, 0xBB, 0xCC,  /* 0x01 = physical 0x00AABBCC */
            0x00, 0x00, 0x00, 0x00,  /* ... */
            /* Physical address 6 */
            0xff, 0xff, 0xff, 0xfb,  /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x01,  /* height = 1 */
            0x00, 0x00, 0x00, 0x00,  /* 0x00 */
            0x00, 0x00, 0x00, 0x00,  /* 0x01 */
            0x00, 0x00, 0x00, 0x01,  /* 0x02 = physical 1 */
            0x00, 0x00, 0x00, 0x00,  /* 0x03 */
            0x00, 0x00, 0x00, 0x00,  /* 0x04 */
            0x00, 0x00, 0x00, 0x00,  /* 0x05 */
            0x00, 0x00, 0x00, 0x00,  /* 0x06 */
            0x00, 0x00, 0x00, 0x00,  /* 0x07 */
            0x00, 0x00, 0x00, 0x00,  /* 0x08 */
            0x00, 0x00, 0x00, 0x00,  /* 0x09 */
            0x00, 0x00, 0x00, 0x00,  /* 0x0A */
    };

    d = obl_create_database(filename);
    d->content = (obl_uint*) contents;
    d->root.address_map_addr = (obl_physical_address) 6;

    result = obl_address_lookup(d, (obl_logical_address) 0x00000201);
    CU_ASSERT(result == (obl_physical_address) 0x00AABBCC);

    /* lookup not found in leaf */
    result = obl_address_lookup(d, (obl_logical_address) 0x0000020A);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

    /* lookup not found in branch */
    result = obl_address_lookup(d, (obl_logical_address) 0x00000301);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

    obl_destroy_database(d);
}

#define AL_SIZE ((1 + CHUNK_SIZE) * sizeof(obl_uint))
void test_assign_leaf(void)
{
    struct obl_database *d;
    char content[AL_SIZE] = {
            0xff, 0xff, 0xff, 0xfb, /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00, /* depth */
            0x00, 0x00, 0x00, 0x00, /* 0x00 */
            0
    };
    char expected[AL_SIZE] = {
            0xff, 0xff, 0xff, 0xfb, /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00, /* depth */
            0x00, 0x00, 0x00, 0x00, /* 0x00 */
            0x00, 0x00, 0xAA, 0xBB, /* 0x01 */
            0
    };

    d = obl_create_database(filename);
    d->content = (obl_uint*) content;
    d->root.address_map_addr = (obl_physical_address) 0;

    obl_address_assign(d,
            (obl_logical_address) 0x01,
            (obl_physical_address) 0x0000AABB);

    CU_ASSERT(memcmp(content, expected, AL_SIZE) == 0);

    obl_destroy_database(d);
}

#define AB_SIZE (1 + (2 * (1 + CHUNK_SIZE)))
void test_assign_branch(void)
{
    struct obl_database *d;
    obl_uint content[AB_SIZE] = { 0x00 };
    obl_uint expected[AB_SIZE] = { 0x00 };
    int i;

    /* leaf @ physical 1 */
    content[1] = writable_uint(OBL_ADDRTREEPAGE_SHAPE_ADDR);
    content[2] = writable_uint((obl_uint) 0);

    /* branch [root] @ physical CHUNK_SIZE + 2 */
    content[CHUNK_SIZE + 2] =
            writable_uint(OBL_ADDRTREEPAGE_SHAPE_ADDR);
    content[CHUNK_SIZE + 3] = writable_uint((obl_uint) 1);
    content[CHUNK_SIZE + 4 + 6] = writable_uint((obl_uint) 1);

    memcpy(expected, content, AB_SIZE);

    /* Assign in leaf @ physical 1, index 0x0A */
    expected[13] = writable_uint((obl_uint) 0x00AA00BB);

    d = obl_create_database(filename);
    d->root.address_map_addr = (obl_physical_address) (CHUNK_SIZE + 2);
    d->content = content;

    obl_address_assign(d,
            (obl_logical_address) 0x0000060A,
            (obl_physical_address) 0x00AA00BB);

    CU_ASSERT(memcmp(content, expected, AB_SIZE) == 0);

    obl_destroy_database(d);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_addressmap_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("addressmap", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    ADD_TEST(test_map_leaf);
    ADD_TEST(test_map_branch);
    ADD_TEST(test_assign_leaf);
    ADD_TEST(test_assign_branch);

    return pSuite;
}
