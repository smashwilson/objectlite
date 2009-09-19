/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for obl_object creation, access, and manipulation API.
 */

#include "CUnit/Headers/Basic.h"

void test_create_integer(void)
{
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_object_suite(void)
{
  CU_pSuite pSuite = NULL;

  pSuite = CU_add_suite("object", NULL, NULL);
  if( pSuite == NULL ) {
    return NULL;
  }

  if(
     (CU_add_test(pSuite, "Create an INTEGER object.", test_create_integer) == NULL)
     ) {
    return NULL;
  }

  return pSuite;
}
