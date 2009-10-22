/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Address tree page objects are internal ObjectLite structures that store the
 * mapping between the obl_logical_address and obl_physical_address spaces.
 * Although the structure is described here for completeness, in practice,
 * address queries and assignments use direct database reads and writes for
 * efficiency.  The file addressmap.h describes the actual address map
 * operations.
 */

#ifndef ADDRTREEPAGE_H
#define ADDRTREEPAGE_H

#include "constants.h"
#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * These structures are used internally to implement the logical to physical
 * address mapping.  See addressmap.h.
 */
struct obl_addrtreepage_storage {

    /** Position of the page within the tree.  Leaves have a height of 0. */
    obl_uint height;

    /**
     * At a leaf, this set contains the target physical addresses.  On a branch,
     * this set maps to the next layer of obl_addrtreepage objects, or
     * OBL_PHYSICAL_UNASSIGNED if no addresses have been mapped in that range.
     */
    obl_physical_address contents[CHUNK_SIZE];
};

/**
 * Address tree page creation.
 *
 * \param d The database to create the page within.
 * \param depth This page's height within the tree; leaves have a height of 0.
 * \return A newly allocated address tree page object with contents of
 *      OBL_PHYSICAL_UNASSIGNED.
 */
struct obl_object *obl_create_addrtreepage(struct obl_database *d,
        obl_uint depth);

#endif /* ADDRTREEPAGE_H */
