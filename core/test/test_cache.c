/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Unit tests for the LRU cache implemented by "cache.c".
 */

#include "CUnit/Headers/Basic.h"

#include "cache.h"

/* A macro to make cache cleanup less repetitive. */
#define ISNT_NULL(pointer, message) \
  if( (pointer) == NULL ) { CU_FAIL(message); obl_cache_destroy(cache); return; }

void test_initialize_cache(void)
{
  obl_cache *cache;
  int bucket_index;

  cache = obl_cache_create(10, 100);
  CU_ASSERT_FATAL(cache != NULL);
  CU_ASSERT(cache->max_size == 100);
  CU_ASSERT(cache->current_size == 0);
  CU_ASSERT(cache->bucket_count == 10);

  for( bucket_index = 0; bucket_index < cache->bucket_count; bucket_index++ ) {
    CU_ASSERT(cache->buckets[bucket_index] == NULL);
  }

  CU_ASSERT(cache->oldest == NULL);
  CU_ASSERT(cache->youngest == NULL);

  obl_cache_destroy(cache);
}

void test_cache_noncolliding(void)
{
  obl_cache *cache;
  obl_object o1, o2;
  obl_cache_entry *entry;
  obl_cache_age_entry *age1, *age2;

  cache = obl_cache_create(10, 100);
  CU_ASSERT_FATAL(cache != NULL);

  /* o1 should be inserted in bucket 1, youngest and oldest. */
  o1.address = (obl_logical_address) 1;
  obl_cache_insert(cache, &o1);

  CU_ASSERT(cache->current_size == 1);
  ISNT_NULL(cache->buckets[1], "Cache bucket 1 not allocated.");
  ISNT_NULL(cache->youngest, "Youngest not allocated.");
  ISNT_NULL(cache->oldest, "Oldest not allocated.");

  entry = cache->buckets[1];
  CU_ASSERT(entry->object == &o1);
  CU_ASSERT(entry->next == NULL);

  age1 = cache->youngest;
  CU_ASSERT(age1 == cache->oldest);
  CU_ASSERT(age1 == entry->age_entry);
  CU_ASSERT(age1->entry == entry);
  CU_ASSERT(age1->older == NULL);
  CU_ASSERT(age1->younger == NULL);

  /* o2 should be inserted in bucket 5 and youngest. */
  o2.address = (obl_logical_address) 55;
  obl_cache_insert(cache, &o2);

  CU_ASSERT(cache->current_size == 2);
  ISNT_NULL(cache->buckets[5], "Bucket 5 not allocated.");

  entry = cache->buckets[5];
  CU_ASSERT(entry->object == &o2);
  CU_ASSERT(entry->next == NULL);

  age2 = cache->youngest;
  CU_ASSERT(age2 == entry->age_entry);
  CU_ASSERT(age2->entry == entry);
  CU_ASSERT(age2->older == age1);
  CU_ASSERT(age2->younger == NULL);

  CU_ASSERT(age1 == cache->oldest);
  CU_ASSERT(age1->older == NULL);
  CU_ASSERT(age1->younger == age2);

  obl_cache_destroy(cache);
}

void test_cache_colliding(void)
{
  obl_cache *cache;
  obl_object o1, o2, o3;
  obl_cache_entry *entry;

  cache = obl_cache_create(10, 100);
  CU_ASSERT_FATAL(cache != NULL);

  /* o1 should be inserted into bucket 2. */
  o1.address = (obl_logical_address) 42;
  obl_cache_insert(cache, &o1);
  ISNT_NULL(cache->buckets[2], "Unable to allocate bucket 2.");

  entry = cache->buckets[2];
  CU_ASSERT(entry->object == &o1);
  CU_ASSERT(entry->next == NULL);

  /* o2 should also be inserted into bucket 2.  The bucket linked lists should
     be sorted, so o2 should be at the list head and o1 should be linked
     after. */
  o2.address = (obl_logical_address) 12;
  obl_cache_insert(cache, &o2);
  entry = cache->buckets[2];
  CU_ASSERT(entry->object == &o2);
  ISNT_NULL(entry->next, "Lost the original bucket.");

  entry = (obl_cache_entry*) entry->next;
  CU_ASSERT(entry->object == &o1);
  CU_ASSERT(entry->next == NULL);

  /* o3 should go into bucket 2, between o2 and o1. */
  o3.address = (obl_logical_address) 32;
  obl_cache_insert(cache, &o3);
  entry = cache->buckets[2];
  CU_ASSERT(entry->object == &o2);
  entry = (obl_cache_entry*) entry->next;
  CU_ASSERT(entry->object == &o3);
  entry = (obl_cache_entry*) entry->next;
  CU_ASSERT(entry->object == &o1);
  CU_ASSERT(entry->next == NULL);

  obl_cache_destroy(cache);
}

void test_quiet_cache_retrieval(void)
{
  obl_cache *cache;
  obl_object o[30];
  obl_object *result;
  int index;

  cache = obl_cache_create(10, 100);
  CU_ASSERT_FATAL(cache != NULL);

  /* Assign the objects o logical addresses of 100, 101, ... 129 and insert them
     into the cache. */
  for( index = 0; index < 30; index++ ) {
    o[index].address = (obl_logical_address) index + 100;
    obl_cache_insert(cache, &o[index]);
  }

  /* Retrieve object 102 (which should be at its bucket head). */
  result = obl_cache_get_quietly(cache, 102);
  CU_ASSERT(result == &o[2]);

  /* Retrieve object 117 (which should be in the middle of its bucket). */
  result = obl_cache_get_quietly(cache, 117);
  CU_ASSERT(result == &o[17]);

  /* Retrieve object 125 (which should be at the bucket tail). */
  result = obl_cache_get_quietly(cache, 125);
  CU_ASSERT(result == &o[25]);

  /* Attempt to retrieve object 200 (which isn't in the cache at all). */
  result = obl_cache_get_quietly(cache, 200);
  CU_ASSERT(result == NULL);

  /* The youngest and oldest pointers should remain untouched through all of
     this. */
  CU_ASSERT(cache->youngest->entry->object == &o[29]);
  CU_ASSERT(cache->oldest->entry->object == &o[0]);

  obl_cache_destroy(cache);
}

void test_cache_retrieval(void)
{
  obl_cache *cache;
  obl_object o[30];
  obl_object *result;
  int index;

  cache = obl_cache_create(10, 100);
  CU_ASSERT_FATAL(cache != NULL);

  /* Insert thirty objects with addresses 100 to 129. */
  for( index = 0; index < 30; index++ ) {
    o[index].address = 100 + index;
    obl_cache_insert(cache, &o[index]);
  }

  CU_ASSERT(cache->youngest->entry->object == &o[29]);
  CU_ASSERT(cache->oldest->entry->object == &o[0]);

  result = obl_cache_get(cache, 115);
  CU_ASSERT(result != NULL);

  /* Look, ma, five levels of pointer indirection!  Eh, it's in a unit test, and
     *I* think it's clear enough.  Eh?  Eh? */
  CU_ASSERT(cache->youngest->entry->object == &o[15]);
  CU_ASSERT(cache->youngest->older->entry->object == &o[29]);
  CU_ASSERT(cache->oldest->entry->object == &o[0]);

  result = obl_cache_get(cache, 100);
  CU_ASSERT(result != NULL);

  CU_ASSERT(cache->youngest->entry->object == &o[0]);
  CU_ASSERT(cache->youngest->older->entry->object == &o[15]);
  CU_ASSERT(cache->youngest->older->older->entry->object == &o[29]);
  CU_ASSERT(cache->oldest->entry->object == &o[1]);

  obl_cache_destroy(cache);
}

void test_cache_deletion(void)
{
  obl_cache *cache;
  obl_object o[10];
  obl_cache_entry *current_entry;
  obl_cache_age_entry *current_age;
  int index, count;

  cache = obl_cache_create(5, 100);
  CU_ASSERT_FATAL(cache != NULL);

  for( index = 0; index < 10; index++ ) {
    o[index].address = (obl_logical_address) 100 + index;
    obl_cache_insert(cache, &o[index]);
  }

  CU_ASSERT(cache->current_size == 10);
  CU_ASSERT(cache->youngest->entry->object == &o[9]);
  CU_ASSERT(cache->oldest->entry->object == &o[0]);

  obl_cache_delete(cache, &o[5]);
  CU_ASSERT(cache->current_size == 9);

  /* Ensure that object o[5] is missing from cache bucket 0. */
  current_entry = cache->buckets[0];
  ISNT_NULL(current_entry, "Cache bucket not allocated.");
  CU_ASSERT(current_entry->object == &o[0]);
  CU_ASSERT(current_entry->next == NULL);

  /* Ensure that object o[5] is missing from the age list. */
  current_age = cache->youngest;
  for( index = 9; index >= 0; index-- ) {
    if( index != 5 ) {
      ISNT_NULL(current_age, "Age list entry missing.");
      CU_ASSERT(current_age->entry->object == &o[index]);
      current_age = (obl_cache_age_entry*) current_age->older;
    }
  }
  CU_ASSERT(current_age == NULL);

  obl_cache_destroy(cache);
}

void test_cache_overfill(void)
{
  obl_cache *cache;
  obl_object o[101];
  int index;

  cache = obl_cache_create(30, 100);
  CU_ASSERT_FATAL(cache != NULL);

  for( index = 0; index < 100; index++ ) {
    o[index].address = (obl_logical_address) index + 100;
    obl_cache_insert(cache, &o[index]);
  }
  CU_ASSERT(cache->current_size == 100);

  o[100].address = (obl_logical_address) 200;
  obl_cache_insert(cache, &o[100]);

  CU_ASSERT(cache->current_size == 100);
  CU_ASSERT(obl_cache_get_quietly(cache, 100) == NULL);
  for( index = 1; index < 101; index++ ) {
    CU_ASSERT(obl_cache_get_quietly(cache, 101) != NULL);
  }
  CU_ASSERT(cache->youngest->entry->object == &o[100]);
  CU_ASSERT(cache->oldest->entry->object == &o[1]);

  obl_cache_destroy(cache);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_cache_suite(void)
{
  CU_pSuite pSuite = NULL;

  pSuite = CU_add_suite("cache", NULL, NULL);
  if( pSuite == NULL ) {
    return NULL;
  }

  if(
     (CU_add_test(pSuite, "Initialize and destroy a cache.", test_initialize_cache) == NULL) ||
     (CU_add_test(pSuite, "Insert non-colliding values into a cache.", test_cache_noncolliding) == NULL) ||
     (CU_add_test(pSuite, "Insert colliding values into a cache.", test_cache_colliding) == NULL) ||
     (CU_add_test(pSuite, "Cache queries that don't modify entry age.", test_quiet_cache_retrieval) == NULL) ||
     (CU_add_test(pSuite, "Cache queries that do modify entry age.", test_cache_retrieval) == NULL) ||
     (CU_add_test(pSuite, "Removing entries manually from the cache.", test_cache_deletion) == NULL) ||
     (CU_add_test(pSuite, "Automatically removing the oldest element.", test_cache_overfill) == NULL)
     ) {
    return NULL;
  }

  return pSuite;
}
