/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implementation of a fixed-size LRU object cache.  obl_object structures are
 * stored here keyed by logical address.  Additions beyond the configured
 * maximum size will cause the oldest object to be removed.
 */

#include "cache.h"

#include "log.h"
#include "error.h"
#include "database.h"

/*
 * Internal functions prototypes.
 */

static int bucket_for_address(obl_cache *cache, obl_logical_address address);

static void insert_in_bucket(obl_cache *cache, int bucket_index,
        obl_cache_entry *entry);

static obl_cache_entry *lookup_address(obl_cache *cache,
        obl_logical_address address, obl_cache_entry **previous);

static void remove_age_entry(obl_cache *cache, obl_cache_age_entry *age);

static void make_youngest(obl_cache *cache, obl_cache_age_entry *age);

/*
 * External functions definitions.
 */

obl_cache *obl_create_cache(int bucket_count, int max_size)
{
    obl_cache *cache;
    int bucket_index;

    cache = (obl_cache*) malloc(sizeof(obl_cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->max_size = max_size;
    cache->current_size = 0;
    cache->bucket_count = bucket_count;

    /* Allocate the cache buckets and initialize them to NULL. */
    cache->buckets = (obl_cache_entry**) malloc(sizeof(obl_cache_entry*)
            * cache->bucket_count);
    if (cache->buckets == NULL) {
        free(cache);
        return NULL;
    }
    for (bucket_index = 0; bucket_index < cache->bucket_count; bucket_index++) {
        cache->buckets[bucket_index] = NULL;
    }

    cache->oldest = NULL;
    cache->youngest = NULL;

    return cache;
}

void obl_destroy_cache(obl_cache *cache)
{
    int bucket_index;
    obl_cache_entry *current, *previous;
    obl_cache_age_entry *current_age, *previous_age;

    for (bucket_index = 0; bucket_index < cache->bucket_count; bucket_index++) {
        current = cache->buckets[bucket_index];
        while (current != NULL) {
            previous = current;
            current = current->next;
            free(previous);
        }
    }
    free(cache->buckets);

    current_age = cache->youngest;
    while (current_age != NULL) {
        previous_age = current_age;
        current_age = current_age->older;
        free(previous_age);
    }

    free(cache);
}

void obl_cache_insert(obl_cache *cache, obl_object *object)
{
    obl_cache_entry *entry;
    obl_cache_age_entry *age;
    int bucket;

    entry = (obl_cache_entry*) malloc(sizeof(obl_cache_entry));
    if (entry == NULL) {
        obl_report_error(cache->database, OUT_OF_MEMORY,
                "Unable to allocate cache entry.");
        return;
    }
    entry->object = object;
    entry->next = NULL;

    age = (obl_cache_age_entry*) malloc(sizeof(obl_cache_age_entry));
    if (entry == NULL) {
        obl_report_error(cache->database, OUT_OF_MEMORY,
                "Unable to allocate cache age list entry.");
        return;
    }
    age->entry = entry;
    age->older = NULL;
    age->younger = NULL;
    entry->age_entry = age;

    cache->current_size++;

    bucket = bucket_for_address(cache, object->logical_address);
    insert_in_bucket(cache, bucket, entry);
    make_youngest(cache, age);

    while (cache->current_size > cache->max_size) {
        obl_cache_delete(cache, cache->oldest->entry->object);
    }
}

void obl_cache_delete(obl_cache *cache, obl_object *object)
{
    obl_cache_delete_at(cache, object->logical_address);
}

void obl_cache_delete_at(obl_cache *cache, obl_logical_address address)
{
    obl_cache_entry *previous_bucket, *found_bucket;
    obl_cache_age_entry *found_age;

    found_bucket = lookup_address(cache, address, &previous_bucket);
    if (found_bucket == NULL) {
        /* Entry not found. */
        return;
    }

    /* Remove the entry from its cache bucket. */
    if (previous_bucket != NULL) {
        previous_bucket->next = found_bucket->next;
    } else {
        cache->buckets[bucket_for_address(cache, address)] = found_bucket->next;
    }

    /* Remove the age entry from the age list. */
    remove_age_entry(cache, found_bucket->age_entry);

    /* Free the entry memory. */
    free(found_bucket->age_entry);
    free(found_bucket);

    cache->current_size--;
}

obl_object *obl_cache_get(obl_cache *cache, obl_logical_address address)
{
    obl_cache_entry *result;

    result = lookup_address(cache, address, NULL);
    if (result == NULL) {
        return NULL;
    }

    /* Promote this entry's age entry to the front of the age list. */
    remove_age_entry(cache, result->age_entry);
    make_youngest(cache, result->age_entry);

    return result->object;
}

obl_object *obl_cache_get_quietly(obl_cache *cache, obl_logical_address address)
{
    obl_cache_entry *result;

    result = lookup_address(cache, address, NULL);

    if (result == NULL) {
        return NULL;
    } else {
        return result->object;
    }
}

/*
 * Internal function definitions.
 */

/*
 * Return the bucket index that an incoming object should be assigned to.  Given
 * an even distribution of object logical addresses (the hash key), these should
 * be evenly distributed within the interval [0..bucket_count].
 */
static int bucket_for_address(obl_cache *cache, obl_logical_address address)
{
    return (int) address % cache->bucket_count;
}

/*
 * Perform the cache insertion, given a newly allocated entry and a bucket
 * index.  Each bucket is either empty (NULL) or a linked list of existing
 * entries ordered by object address.
 */
static void insert_in_bucket(obl_cache *cache, int bucket_index,
        obl_cache_entry *entry)
{
    obl_cache_entry *head, *current, *previous;
    obl_logical_address address;

    address = entry->object->logical_address;
    head = cache->buckets[bucket_index];
    previous = NULL;
    current = head;

    while (current != NULL && current->object->logical_address <= address) {
        previous = current;
        current = current->next;
    }

    if (previous == NULL) {
        /* Insert at the list front.  Point the bucket pointer here and the new
         entry's next pointer at the existing list front. */
        entry->next = head;
        cache->buckets[bucket_index] = entry;
    } else {
        /* Insert somewhere in the list middle.  current is the entry that should
         fall immediately after the new one; previous is the entry before.  If we
         are at the list end, current may also be NULL. */
        previous->next = entry;
        entry->next = current;
    }
}

/*
 * Return the bucket entry containing the object at address, if one is present.
 * Return NULL if it isn't.  On a successful lookup, if previous is not NULL, it
 * will also be set to point to the previous entry.
 */
static obl_cache_entry *lookup_address(obl_cache *cache,
        obl_logical_address address, obl_cache_entry **previous)
{
    obl_cache_entry *head, *current, *last;
    int bucket_index;

    bucket_index = bucket_for_address(cache, address);
    head = cache->buckets[bucket_index];
    if (previous != NULL) {
        *previous = NULL;
    }
    current = head;

    while (current != NULL && current->object->logical_address <= address) {
        if (current->object->logical_address == address) {
            return current;
        }

        if (previous != NULL) {
            *previous = current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * Remove an age entry from the cache, either to promote it to the front, or to
 * correspond to an element removal.  This does not free its memory.
 */
static void remove_age_entry(obl_cache *cache, obl_cache_age_entry *age)
{
    struct _obl_cache_age_entry *older;
    struct _obl_cache_age_entry *younger;

    older = age->older;
    younger = age->younger;

    if (cache->youngest == age) {
        cache->youngest = older;
    }
    if (cache->oldest == age) {
        cache->oldest = younger;
    }

    if (older != NULL) {
        older->younger = younger;
    }
    if (younger != NULL) {
        younger->older = older;
    }

    age->older = NULL;
    age->younger = NULL;
}

/*
 * Add the provided age entry to the cache as the most recently used thing.
 */
static void make_youngest(obl_cache *cache, obl_cache_age_entry *age)
{
    age->older = cache->youngest;
    if (cache->youngest != NULL) {
        cache->youngest->younger = age;
    }

    cache->youngest = age;

    if (cache->oldest == NULL) {
        cache->oldest = age;
    }
}
