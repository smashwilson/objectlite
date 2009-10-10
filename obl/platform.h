/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Platform-specific compile-time options.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/*
 * A single ObjectLite "word".  This is the smallest unit in which anything can be addressed
 * by, read from, or written to the database, and is used to store "small" signed numeric
 * quantities.  obl_words and obl_uwords are often used throughout the ObjectLite API to
 * communicate things like indices, offsets, or sizes.
 */
typedef int32_t obl_int;

/*
 * The same storage as an obl_int, but unsigned.
 */
typedef uint32_t obl_uint;

/*
 * Width of an address unit, physical or logical.
 */
typedef obl_uint obl_address;

/* Used to specify a physical word address within the .obl file. */
typedef obl_address obl_physical_address;

/*
 * Used to specify the logical address of a remote object that's stored
 * elsewhere in the database.
 */
typedef obl_address obl_logical_address;

/* 
 * The local networking libraries include byte-order conversion functions such
 * as ntohs() and ntohl().
 */
#ifdef WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

#endif
