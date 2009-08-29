/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the obl_database struct and the operations provide on it.
 * This will constitute a fair amount of the public API.
 */

#include "CUnit/Headers/Basic.h"

#include "database.h"
#include "constants.h"

void test_initialize_database(void)
{
  obl_database *database;
  obl_cache *cache;

  database = obl_create_database("foo.obl");
  CU_ASSERT_FATAL(database != NULL);

  if( database->filename != NULL ) {
    CU_ASSERT(strcmp(database->filename, "foo.obl") == 0);
  }

  if( database->cache != NULL ) {
    cache = database->cache;
    CU_ASSERT(cache->current_size == 0);
    CU_ASSERT(cache->max_size == DEFAULT_CACHE_SIZE);
    CU_ASSERT(cache->bucket_count == DEFAULT_CACHE_BUCKETS);
    CU_ASSERT(cache->buckets != NULL);
  } else {
    CU_FAIL("Cache not allocated.");
  }

  CU_ASSERT(obl_database_ok(database));

  obl_destroy_database(database);
}

void test_report_error(void)
{
  obl_database *database;

  database = obl_create_database("foo.obl");
  CU_ASSERT_FATAL(database != NULL);
  CU_ASSERT(obl_database_ok(database));

  obl_report_error(database, OUT_OF_MEMORY, "A sample error message.");

  CU_ASSERT( ! obl_database_ok(database) );
  CU_ASSERT(strcmp(database->last_error.message, "A sample error message.") == 0);
  CU_ASSERT(database->last_error.code == OUT_OF_MEMORY);

  obl_clear_error(database);

  CU_ASSERT(obl_database_ok(database));
  CU_ASSERT(database->last_error.message == NULL);
  CU_ASSERT(database->last_error.code == OK);

  obl_destroy_database(database);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_database_suite(void)
{
  CU_pSuite pSuite = NULL;

  pSuite = CU_add_suite("database", NULL, NULL);
  if( pSuite == NULL ) {
    return NULL;
  }

  if(
     (CU_add_test(pSuite, "Initialize the database structure.", test_initialize_database) == NULL) ||
     (CU_add_test(pSuite, "Reporting and clearing errors.", test_report_error) == NULL)
     ) {
    return NULL;
  }

  return pSuite;
}
