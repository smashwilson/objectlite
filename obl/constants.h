/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Compile-time constants for use throughout the system.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * The number of words to allocate to an object chunk.  Must be a power of two,
 * or the address map breaks (among other things).  But who would want to change
 * it to such a messy number anyway?
 */
#define CHUNK_SIZE 256

/**
 * We *could* calculate this on the fly off of CHUNK_SIZE.  It's a lot easier
 * to do it manually once, and record the result here.
 */
#define CHUNK_SIZE_LOG2 9

/**
 * Default number of buckets in the cache.  This should be set to a prime number
 * relatively close to DEFAULT_CACHE_SIZE.
 */
#define DEFAULT_CACHE_BUCKETS 1021

/**
 * Default depth to automatically follow stubs.
 */
#define DEFAULT_STUB_DEPTH 4

/**
 * Extend the database file by this many bytes each time an
 * allocation is attempted after the end.
 */
#define DEFAULT_GROWTH_SIZE 4096

#endif
