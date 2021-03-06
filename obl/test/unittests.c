/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Master file and main() function for the ObjectLite unit testing suite.
 */

#include "CUnit/Basic.h"

#include "database.h"

/*
 * Prototypes for the initialization methods of each test_xyz module.
 */

CU_pSuite initialize_io_suite(void);
CU_pSuite initialize_database_suite(void);
CU_pSuite initialize_object_suite(void);
CU_pSuite initialize_addressmap_suite(void);
CU_pSuite initialize_allocator_suite(void);
CU_pSuite initialize_session_suite(void);

/*
 * Prototypes for non-CUnit test cases.  Manually call these from main() to
 * run them.
 */

void stress_test_set(unsigned long insert_count,
        unsigned long delete_count);

/*
 * Initialize the CUnit testing suite, allowing each testing module to
 * contribute a test suite.  Execute the resulting suites.
 */
int main()
{
    obl_startup();

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    if (
            (initialize_io_suite() == NULL) ||
            (initialize_database_suite() == NULL) ||
            (initialize_object_suite() == NULL) ||
            (initialize_addressmap_suite() == NULL) ||
            (initialize_allocator_suite() == NULL) ||
            (initialize_session_suite() == NULL)
    ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    obl_shutdown();
    return CU_get_error();
}
