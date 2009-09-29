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
 * Width of an address unit, physical or logical.  Ideally, compiling a build with
 * 64 bit addresses should be as simple as changing this typedef.
 */
typedef uint32_t obl_address;

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
