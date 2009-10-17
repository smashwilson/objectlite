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

const static char *filename = "addrmap.obl";

void test_map_leaf(void)
{
    struct obl_database *d;
    struct obl_object *addrtreepage;
    obl_physical_address result;
    const char contents[] = {
            0x00, 0x00, 0x00, 0x00, /* padding word */
            /* Physical address 1 */
            0xff, 0xff, 0xff, 0xfc, /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00, /* height = 0 */
            0x00, 0x00, 0x00, 0x00, /* 0x00 */
            0x00, 0x00, 0x1A, 0x2B, /* 0x01 */
    };

    d = obl_create_database(filename);
    d->content = (obl_uint*) contents;
    d->root.address_map = (obl_physical_address) 1;

    result = obl_address_for(d, (obl_logical_address) 1);
    CU_ASSERT(result == (obl_physical_address) 0x1A2B);

    result = obl_address_for(d, (obl_logical_address) 0x400);
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
            0xff, 0xff, 0xff, 0xfc,  /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
            0x00, 0x00, 0x00, 0x00,  /* height = 0 */
            0x00, 0x00, 0x00, 0x00,  /* 0x00 = OBL_PHYSICAL_UNASSIGNED */
            0x00, 0xAA, 0xBB, 0xCC,  /* 0x01 = physical 0x00AABBCC */
            0x00, 0x00, 0x00, 0x00,  /* ... */
            /* Physical address 6 */
            0xff, 0xff, 0xff, 0xfc,  /* OBL_ADDRTREEPAGE_SHAPE_ADDR */
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
    d->root.address_map = (obl_physical_address) 6;

    result = obl_address_for(d, (obl_logical_address) 0x00000201);
    CU_ASSERT(result == (obl_physical_address) 0x00AABBCC);

    /* lookup not found in leaf */
    result = obl_address_for(d, (obl_logical_address) 0x0000020A);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

    /* lookup not found in branch */
    result = obl_address_for(d, (obl_logical_address) 0x00000301);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

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

    if (
        (CU_add_test(pSuite,
                "test_map_leaf",
                test_map_leaf) == NULL) ||
        (CU_add_test(pSuite,
                "test_map_branch",
                test_map_branch) == NULL)
    ) {
        return NULL;
    }

    return pSuite;
}
