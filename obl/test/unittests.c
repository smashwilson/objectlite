/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Master file and main() function for the ObjectLite unit testing suite.
 */

#include "CUnit/Basic.h"

#include "database.h"
#include "set.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 * Prototypes for the initialization methods of each test_xyz module.
 */

CU_pSuite initialize_io_suite(void);
CU_pSuite initialize_cache_suite(void);
CU_pSuite initialize_database_suite(void);
CU_pSuite initialize_object_suite(void);
CU_pSuite initialize_addressmap_suite(void);
CU_pSuite initialize_allocator_suite(void);

/*
 * A placeholder for those times that I want to run other temporary testing
 * code that isn't a unit test.  exit() at the end for best effect.
 */

void independent_test()
{
    struct obl_set *set;
    struct obl_database *d;
    obl_logical_address addr;
    int black_height;

    srand(time(NULL));

    d = obl_create_database("foo.obl");
    set = obl_create_set(&logical_address_keyfunction);

    printf("Starting...");
    for (addr = 0; addr < 10000000; addr++) {
        struct obl_object *o;

        o = obl_create_integer(d, (obl_uint) addr);
        o->logical_address = (obl_logical_address) addr;

        obl_set_insert(set, o);
        if (addr % 1000 == 0) {
            printf("\rInserted <%8lu>", addr);
        }
    }
    printf("\n");

    for (addr = 0; addr < 10000; addr++) {
        struct obl_object o;
        o.logical_address = (obl_logical_address) rand();

        obl_set_remove(set, &o);

        if (addr % 1000 == 0) {
            printf("\rDeleted <%8lu>", addr);
        }
    }
    printf("\n");

    black_height = obl_set_verify(set);
    if (black_height == 0) {
        fprintf(stderr, "Set is invalid!");
        exit(1);
    } else {
        printf("Set is valid! Black height: %d\n", black_height);
    }

    exit(0);
}

/*
 * Initialize the CUnit testing suite, allowing each testing module to
 * contribute a test suite.  Execute the resulting suites.
 */
int main()
{
    independent_test();

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    if (
            (initialize_io_suite() == NULL) ||
            (initialize_cache_suite() == NULL) ||
            (initialize_database_suite() == NULL) ||
            (initialize_object_suite() == NULL) ||
            (initialize_addressmap_suite() == NULL) ||
            (initialize_allocator_suite() == NULL)
    ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
