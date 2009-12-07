/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the logical to physical address mapping.
 */

#include "CUnit/Basic.h"

#include "addressmap.h"

#include "storage/object.h"
#include "database.h"
#include "set.h"
#include "session.h"
#include "unitutilities.h"

void test_map_leaf(void)
{
    struct obl_database *d;
    obl_physical_address result;

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, 1, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_CHAR(d->content, 2, 0x00, 0x00, 0x00, 0x00); /* height = 0 */
    SET_CHAR(d->content, 4, 0x00, 0x00, 0x1A, 0x2B); /* entry 0x01 */

    d->root.address_map_addr = (obl_physical_address) 1;

    result = obl_address_lookup(d, (obl_logical_address) 1);
    CU_ASSERT(result == (obl_physical_address) 0x1A2B);

    result = obl_address_lookup(d, (obl_logical_address) 0x400);
    CU_ASSERT(result == OBL_PHYSICAL_UNASSIGNED);

    obl_close_database(d);
}

void test_map_branch(void)
{
    struct obl_database *d;
    obl_physical_address result;

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, 1, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_CHAR(d->content, 2, 0x00, 0x00, 0x00, 0x00); /* page height */
    SET_CHAR(d->content, 4, 0x00, 0xAA, 0xBB, 0xCC); /* 0x01 = addr */

    SET_UINT(d->content, 260, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_CHAR(d->content, 261, 0x00, 0x00, 0x00, 0x01); /* height = 1 */
    SET_CHAR(d->content, 264, 0x00, 0x00, 0x00, 0x01); /* 0x02 = physical 1 */

    d->root.address_map_addr = (obl_physical_address) 260;

    result = obl_address_lookup(d, (obl_logical_address) 0x00000201);
    CU_ASSERT(result == (obl_physical_address) 0x00AABBCC);

    /* lookup not found in leaf */
    result = obl_address_lookup(d, (obl_logical_address) 0x0000020A);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

    /* lookup not found in branch */
    result = obl_address_lookup(d, (obl_logical_address) 0x00000301);
    CU_ASSERT(result == OBL_LOGICAL_UNASSIGNED);

    obl_close_database(d);
}

void test_assign_leaf(void)
{
    struct obl_database *d;
    struct obl_session *s;
    obl_uint expected[10] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, 0, OBL_ADDRTREEPAGE_SHAPE_ADDR);

    d->root.address_map_addr = (obl_physical_address) 0;

    s = obl_create_session(d);

    obl_address_assign(s,
            (obl_logical_address) 0x01,
            (obl_physical_address) 0x0000AABB);

    SET_UINT(expected, 0, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_CHAR(expected, 3, 0x00, 0x00, 0xAA, 0xBB); /* 0x01 in page*/

    CU_ASSERT(memcmp(d->content, expected, 10 * sizeof(obl_uint)) == 0);

    obl_destroy_session(s);
    obl_close_database(d);
}

#define AB_SIZE (1 + (2 * (1 + CHUNK_SIZE)))
void test_assign_branch(void)
{
    struct obl_database *d;
    struct obl_session *s;
    obl_uint expected[AB_SIZE] = { 0 };

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, 1, OBL_ADDRTREEPAGE_SHAPE_ADDR);

    SET_UINT(d->content, CHUNK_SIZE + 2, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(d->content, CHUNK_SIZE + 3, 1);
    SET_UINT(d->content, CHUNK_SIZE + 4 + 6, 1);

    memcpy(expected, d->content, AB_SIZE * sizeof(obl_uint));
    SET_UINT(expected, 13, (obl_uint) 0x00AA00BB);

    d->root.address_map_addr = (obl_physical_address) (CHUNK_SIZE + 2);

    s = obl_create_session(d);

    obl_address_assign(s,
            (obl_logical_address) 0x0000060A,
            (obl_physical_address) 0x00AA00BB);

    CU_ASSERT(memcmp(d->content, expected, AB_SIZE * sizeof(obl_uint)) == 0);

    obl_destroy_session(s);
    obl_close_database(d);
}

#define CL_SIZE (1 + 2 * (CHUNK_SIZE + 2))
void test_create_leaf(void)
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *allocator, *next_physical;
    obl_uint expected[CL_SIZE] = { 0 };
    const obl_uint branch = 1;
    const obl_uint leaf = branch + CHUNK_SIZE + 2;

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, branch, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(d->content, branch + 1, (obl_uint) 1);

    memcpy(expected, d->content, CL_SIZE * sizeof(obl_uint));
    SET_UINT(expected, branch + 2 + 4, leaf);
    SET_UINT(expected, leaf, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(expected, leaf + 2 + 3, 0xAABBCCDD);

    d->root.address_map_addr = (obl_physical_address) branch;

    s = obl_create_session(d);

    /* Create an address allocator and prime the read set with it. */
    allocator = obl_create_slotted(
            _obl_at_fixed_address(OBL_ALLOCATOR_SHAPE_ADDR));
    next_physical = obl_create_integer((obl_int) leaf);
    obl_slotted_atcnamed_put(allocator, "next_physical", next_physical);
    allocator->logical_address = (obl_logical_address) 1;
    d->root.allocator_addr = allocator->logical_address;
    obl_set_insert(s->read_set, allocator);

    obl_address_assign(s,
            (obl_logical_address) 0x00000403,
            (obl_physical_address) 0xAABBCCDD);

    CU_ASSERT(memcmp(d->content, expected, CL_SIZE * 4) == 0);

    obl_destroy_object(next_physical);

    obl_destroy_session(s);
    obl_close_database(d);
}

#define CB_SIZE (1 + 3 * (CHUNK_SIZE + 2))
void test_create_branch(void)
{
    struct obl_database *d;
    struct obl_session *s;
    struct obl_object *allocator, *next_physical;
    obl_uint expected[CB_SIZE] = { 0x00 };
    const obl_uint leaf_a = 1;
    const obl_uint branch = leaf_a + CHUNK_SIZE + 2;
    const obl_uint leaf_b = branch + CHUNK_SIZE + 2;

    d = obl_open_defdatabase(NULL);
    wipe(d);

    SET_UINT(d->content, leaf_a, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(d->content, leaf_a + 1, 0);

    memcpy(expected, d->content, CB_SIZE * sizeof(obl_uint));
    SET_UINT(expected, branch, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(expected, branch + 1, 1);
    SET_UINT(expected, branch + 2 + 0, leaf_a);
    SET_UINT(expected, branch + 2 + 1, leaf_b);
    SET_UINT(expected, leaf_b, OBL_ADDRTREEPAGE_SHAPE_ADDR);
    SET_UINT(expected, leaf_b + 1, 0);
    SET_UINT(expected, leaf_b + 2 + 15, 0xADBCCBDA);

    d->root.address_map_addr = (obl_physical_address) 1;

    s = obl_create_session(d);

    /* Create an address allocator and prime the read set with it. */
    allocator = obl_create_slotted(
            _obl_at_fixed_address(OBL_ALLOCATOR_SHAPE_ADDR));
    next_physical = obl_create_integer((obl_int) branch);
    obl_slotted_atcnamed_put(allocator, "next_physical", next_physical);
    allocator->logical_address = (obl_logical_address) 1;
    d->root.allocator_addr = allocator->logical_address;
    obl_set_insert(s->read_set, allocator);

    obl_address_assign(s,
            (obl_logical_address) 0x0000010F,
            (obl_physical_address) 0xADBCCBDA);

    CU_ASSERT(memcmp(d->content, expected, CB_SIZE * 4) == 0);
    CU_ASSERT(d->root.address_map_addr == (obl_physical_address) branch);
    CU_ASSERT(d->root.dirty);

    obl_destroy_object(next_physical);
    obl_destroy_session(s);
    obl_close_database(d);
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
    ADD_TEST(test_create_leaf);
    ADD_TEST(test_create_branch);

    return pSuite;
}
