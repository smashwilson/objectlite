/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file addressmap.h
 *
 * These functions map logical addresses to physical addresses within an
 * ObjectLite database.
 */

#ifndef ADDRESSMAP_H
#define ADDRESSMAP_H

#include "platform.h"

/* Defined in database.h */
struct obl_database;

/* Defined in session.h */
struct obl_session;

/* Defined in object.h */
struct obl_object;

/**
 * Translate a logical address +logical+ into an assigned physical address, or
 * OBL_PHYSICAL_UNASSIGNED if none yet exists.
 *
 * @param d The database in which the lookup shall be performed.
 * @param logical The logical address to translate.
 * @return The physical address mapped to "logical", or OBL_PHYSICAL_UNASSIGNED
 *      if no such address exists.
 */
obl_physical_address obl_address_lookup(struct obl_database *d,
        obl_logical_address logical);

/**
 * Store a mapping between the addresses "logical" and "physical", creating
 * address map tree pages as necessary.  This function modifies the address map
 * directly and should therefore only be invoked by a session in the
 * process of committing changes.
 *
 * @param s The session in which the assignment should be made.  This session
 *      should be preparing to commit.
 * @param logical The logical address to map.
 * @param physical The physical address to map it to.
 */
void obl_address_assign(struct obl_session *s,
        obl_logical_address logical, obl_physical_address physical);

#endif /* ADDRESSMAP_H */
