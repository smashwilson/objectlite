/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Compile-time constants for use throughout the system.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*
 * The number of words to allocate to an object chunk.
 */
#define CHUNK_SIZE 256

/*
 * Default maximum size of the cache.
 */
#define DEFAULT_CACHE_SIZE 1024

/*
 * Default number of buckets in the cache.  This should be set to a prime number
 * relatively close to DEFAULT_CACHE_SIZE.
 */
#define DEFAULT_CACHE_BUCKETS 1021

/*
 * Default depth to automatically follow stubs.
 */
#define DEFAULT_STUB_DEPTH 4

#endif
