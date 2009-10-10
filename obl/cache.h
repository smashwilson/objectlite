/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implementation of a fixed-size LRU object cache.  struct obl_object structures are
 * stored here keyed by logical address.  Additions beyond the configured
 * maximum size will cause the oldest object to be freed from memory.
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>

#include "platform.h"

/* Defined in database.h */
struct obl_database;

/* Defined in object.h */
struct obl_object;

/* Forward declared for circular references with obl_cache_entry. */
struct obl_cache_age_entry;

struct obl_cache_entry {
    struct obl_object *object;
    struct obl_cache_entry *next;
    struct obl_cache_age_entry *age_entry;
};

struct obl_cache_age_entry {
    struct obl_cache_entry *entry;
    struct obl_cache_age_entry *older;
    struct obl_cache_age_entry *younger;
};

struct obl_cache {
    int max_size;
    int current_size;
    int bucket_count;
    struct obl_cache_entry **buckets;
    struct obl_cache_age_entry *oldest;
    struct obl_cache_age_entry *youngest;
    struct obl_database *database;
};

/*
 * Allocate a new cache with the provided maximum size.
 */
struct obl_cache *obl_create_cache(int buckets_size, int max_size);

/*
 * Destroy an obl_cache produced by obl_cache_create.
 */
void obl_destroy_cache(struct obl_cache *cache);

/*
 * Insert a new object into a cache, hashed by its logical address.  Mark it as
 * the youngest cache member.
 */
void obl_cache_insert(struct obl_cache *cache, struct obl_object *object);

/*
 * Remove an object from the cache.  If the object is not present, has no
 * effect.
 */
void obl_cache_delete(struct obl_cache *cache, struct obl_object *object);

/*
 * Remove an object from the cache by address.
 */
void obl_cache_delete_at(struct obl_cache *cache,
        obl_logical_address address);

/*
 * Query the cache for an object at a given address.  If it is found, bump that
 * object to the front of the recency list.  If it is not found, return NULL.
 */
struct obl_object *obl_cache_get(struct obl_cache *cache,
        obl_logical_address address);

/*
 * Query the cache for an object at a given address.  Do not modify the recency
 * list whether it is found or not.
 */
struct obl_object *obl_cache_get_quietly(struct obl_cache *cache,
        obl_logical_address address);

#endif
