/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Platform-specific types and codes.
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
 * Minimum and maximum values that can be assigned to obl_ints and obl_uints.
 */
#define OBL_INT_MIN INT32_MIN
#define OBL_INT_MAX INT32_MAX

#define OBL_UINT_MAX UINT32_MAX

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
 * Valid ranges for the address types.
 */
#define OBL_ADDRESS_MAX OBL_UINT_MAX

/* 
 * The local networking libraries include byte-order conversion functions such
 * as ntohs() and ntohl().
 */
#ifdef WIN32

#include <Winsock2.h>

#else

#include <netinet/in.h>

#endif

/*
 * Memory mapping and unmapping functions: native on POSIX systems, emulated
 * on WIN32.
 */
#ifdef WIN32
#include <sys/types.h>

void *mmap(void *start, size_t length, int prot, int flags, int fd,
        off_t offset);

int munmap(void *start, size_t length);

/* +prot+ and +flags+ constants. */
#ifndef PROT_READ
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 1
#define MAP_FAILED ((void*) -1)
#endif

#else

#include <sys/mman.h>
#include <stdio.h>

#endif

#endif
