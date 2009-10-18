/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * These functions map logical addresses to physical addresses within an
 * ObjectLite database.
 */

#ifndef ADDRESSMAP_H
#define ADDRESSMAP_H

#include "platform.h"

/* Defined in database.h */
struct obl_database;

/* Defined in object.h */
struct obl_object;

/*
 * Translate a logical address +logical+ into an assigned physical address, or
 * OBL_PHYSICAL_UNASSIGNED if none yet exists.
 */
obl_physical_address obl_address_lookup(struct obl_database *d,
        obl_logical_address logical);

/*
 * Store a mapping between the addresses +logical+ and +physical+, creating
 * address map tree pages as necessary.
 */
void obl_address_assign(struct obl_database *d,
        obl_logical_address logical, obl_physical_address physical);

#endif /* ADDRESSMAP_H */
