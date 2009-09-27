/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for obl_object creation, access, and manipulation API.
 */

#include "object.h"

#include <string.h>

#include "CUnit/Headers/Basic.h"

#include "database.h"

void test_create_integer(void)
{
  obl_database *d;
  obl_object *o;

  d = obl_create_database("unit.obl");

  o = obl_create_integer(d, 42);
  CU_ASSERT_FATAL(o != NULL);
  CU_ASSERT(o->database == NULL);
  CU_ASSERT(o->shape == obl_at_address(d, OBL_INTEGER_SHAPE_ADDR));
  CU_ASSERT(obl_integer_value(o) == 42);

  obl_destroy_object(o);
  obl_destroy_database(d);
}

void test_create_string(void)
{
  char *string = "NULL-terminated C string.";
  obl_database *d;
  obl_object *o;

  d = obl_create_database("unit.obl");
  
  o = obl_create_string(d, string, strlen(string));
  CU_ASSERT_FATAL(o != NULL);
  CU_ASSERT(o->database == NULL);
  CU_ASSERT(o->shape == obl_at_address(d, OBL_STRING_SHAPE_ADDR));
  CU_ASSERT(strcmp(obl_string_value(o), string) == 0);

  obl_destroy_object(o);
  obl_destroy_database(d);
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
     (CU_add_test(pSuite, "Create an OBL_INTEGER object.", test_create_integer) == NULL) ||
     (CU_add_test(pSuite, "Create an OBL_STRING object from a C string.", test_create_string) == NULL)
     ) {
    return NULL;
  }

  return pSuite;
}
