/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the functions contained in "io.c".  These functions provide
 * primitive object serialization and deserialization directly to and from
 * open files.
 */

#include "CUnit/Headers/Basic.h"

#include "io.h"

void test_read_integer(void)
{
  CU_ASSERT(0 == 1);
}

int main()
{
  CU_pSuite pSuite = NULL;

  if( CU_initialize_registry() != CUE_SUCCESS ) {
    return CU_get_error();
  }

  pSuite = CU_add_suite("io", NULL, NULL);
  if( pSuite == NULL ) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if(
     (CU_add_test(pSuite, "description", test_read_integer) == NULL)
     ) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
