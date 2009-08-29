/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implementation of a fixed-size LRU object cache.  obl_object structures are
 * stored here keyed by logical address.  Additions beyond the configured
 * maximum size will cause the oldest object to be freed from memory.
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>

#include "object.h"

struct _obl_cache_age_entry;

typedef struct _obl_cache_data_entry {
  obl_object *object;
  struct _obl_cache_entry *next;
  struct _obl_cache_age_entry *age_entry;
} obl_cache_entry;

typedef struct _obl_cache_age_entry {
  obl_cache_entry *entry;
  struct _obl_cache_age_entry *older;
  struct _obl_cache_age_entry *younger;
} obl_cache_age_entry;

typedef struct {
  int max_size;
  int current_size;
  int bucket_count;
  obl_cache_entry **buckets;
  obl_cache_age_entry *oldest;
  obl_cache_age_entry *youngest;
} obl_cache;

/*
 * Allocate a new cache with the provided maximum size.
 */
obl_cache *obl_cache_create(int buckets_size, int max_size);

/*
 * Destroy an obl_cache produced by obl_cache_create.
 */
void obl_cache_destroy(obl_cache *cache);

/*
 * Insert a new object into a cache, hashed by its logical address.  Mark it as
 * the youngest cache member.
 */
void obl_cache_insert(obl_cache *cache, obl_object *object);

/*
 * Remove an object from the cache.  If the object is not present, has no
 * effect.
 */
void obl_cache_delete(obl_cache *cache, obl_object *object);

/*
 * Remove an object from the cache by address.
 */
void obl_cache_delete_at(obl_cache *cache, obl_logical_address address);

/*
 * Query the cache for an object at a given address.  If it is found, bump that
 * object to the front of the recency list.  If it is not found, return NULL.
 */
obl_object *obl_cache_get(obl_cache *cache, obl_logical_address address);

/*
 * Query the cache for an object at a given address.  Do not modify the recency
 * list whether it is found or not.
 */
obl_object *obl_cache_get_quietly(obl_cache *cache, obl_logical_address address);

#endif
