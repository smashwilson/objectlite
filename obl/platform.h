/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Platform-specific types and codes.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <limits.h>

#include "unicode/utypes.h"

/**
 * A single ObjectLite "word".  This is the smallest unit in which anything can
 * be addressed by, read from, or written to the database, and is used to store
 * "small" signed numeric quantities.  obl_words and obl_uwords are often used
 * throughout the ObjectLite API to communicate things like indices, offsets, or
 * sizes.
 */
typedef int32_t obl_int;

/**
 * The same storage size as an obl_int, but unsigned.
 */
typedef uint32_t obl_uint;

/**
 * Minimum and maximum values that can be assigned to obl_ints and obl_uints.
 */
#define OBL_INT_MIN INT32_MIN
#define OBL_INT_MAX INT32_MAX

#define OBL_UINT_MAX UINT32_MAX

/**
 * A useful sentinel value to return from functions that communicate with
 * obl_uint.
 */
#define OBL_SENTINEL OBL_UINT_MAX

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
 * The physical address used to denote that an obl_object is not persisted yet.
 */
#define OBL_PHYSICAL_UNASSIGNED ((obl_physical_address) 0)

/*
 * The logical address used to denote that an obl_object is not persisted yet.
 */
#define OBL_LOGICAL_UNASSIGNED ((obl_logical_address) 0)

/*
 * Valid ranges for the address types.
 */
#define OBL_ADDRESS_MAX OBL_UINT_MAX

/*
 * The functions writable_uint() and readable_uint() use networking library functions
 * ntohl() and htonl() to convert between network-byte order file storage and
 * host-byte order memory storage when necessary.
 *
 * If the obl_uint and unsigned long types ever differ in storage size, these
 * functions will require slightly more bit math to correctly pack and unpack
 * values, although that case will also cause data format compatibility issues.
 * This is not the case on any of our target platforms.
 *
 * These functions will also need to be redefined if we support 64 bit
 * repositories.
 */
#ifdef WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

#define writable_uint(in) htonl(in)
#define readable_uint(in) ((obl_uint) ntohl(in))

#define writable_int(in) htonl(in)
#define readable_int(in) ((obl_int) ntohl(in))

/* A UChar is guaranteed to be 16 bits wide. */
#define writable_UChar(ch) htons(ch)
#define readable_UChar(ch) ((UChar) ntohs(ch))

/* A UChar32 is guaranteed to be 32 bits wide. */
#define writable_UChar32(ch) htonl(ch)
#define readable_UChar32(ch) ((UChar32) ntohl(ch))

/* obl_logical_address should always be the same width as an obl_uint. */
#define writable_logical(in) writable_uint((obl_uint) (in))
#define readable_logical(in) ((obl_logical_address) readable_uint(in))

/* obl_physical_address should always be the same width as an obl_uint. */
#define writable_physical(in) writable_uint((obl_uint) (in))
#define readable_physical(in) ((obl_physical_address) writable_uint(in))

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
#define MAP_SHARED 1
#define MAP_FAILED ((void*) -1)
#endif

#else

#include <sys/mman.h>
#include <stdio.h>

#endif

#endif
