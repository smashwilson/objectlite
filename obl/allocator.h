/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file allocator.h
 *
 * The allocator is responsible for assigning unused logical and physical
 * addresses to newly created objects.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "platform.h"

/* defined in database.h */
struct obl_database;

/* defined in session.h */
struct obl_session;

/**
 * Allocate an unused logical address.
 *
 * @param d The database in which allocation should occur.
 * @return A currently unused obl_logical_address, which will not be reassigned
 *      until garbage collection declares it available.
 */
obl_logical_address obl_allocate_logical(struct obl_database *d);

/**
 * Allocate an unused physical address.  Reserve size space after the
 * allocated address.
 *
 * @param d The database in which allocation should occur.
 * @param size The number of obl_uint-sized blocks to reserve for this object.
 * @return An obl_physical_address that is currently unused, which will now be
 *      reserved until the garbage collector declares it available.
 */
obl_physical_address obl_allocate_physical(struct obl_database *d,
        obl_uint size);

#endif /* ALLOCATOR_H */
