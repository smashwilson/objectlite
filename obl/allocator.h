/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * The allocator is responsible for assigning unused logical and physical
 * addresses to newly created objects.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "platform.h"

/* defined in database.h */
struct obl_database;

/*
 * Allocate an unused logical address.
 */
obl_logical_address obl_allocate_logical(struct obl_database *d);

/*
 * Allocate an unused physical address.  Reserve +size+ space after the
 * allocated address.
 */
obl_physical_address obl_allocate_physical(struct obl_database *d,
        obl_uint size);

#endif /* ALLOCATOR_H */
