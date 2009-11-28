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
 * @param depth This page's height within the tree; leaves have a height of 0.
 * @return A newly allocated address tree page object with contents of
 *      OBL_PHYSICAL_UNASSIGNED.
 */
struct obl_object *obl_create_addrtreepage(struct obl_database *d,
        obl_uint depth);

/**
 * Read an address tree page.  Address tree pages reference each other by
 * physical address (so that they can used during the address lookup process)
 * so tree pages don't respect the +depth+ parameter.
 */
struct obl_object *obl_addrtreepage_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/**
 * Write an address map tree page.
 */
void obl_addrtreepage_write(struct obl_object *treepage, obl_uint *dest);

/**
 * Output the contents of an address tree page.  This will usually be an
 * ungodly wall of text.
 *
 * @param addrtreepage The address tree page to output.
 * @param depth How much farther to recurse into the object structure.  If
 *      depth is zero, output an abbreviated (single-line) representation of
 *      this object.
 * @param indent Level of indentation.
 */
void obl_addrtreepage_print(struct obl_object *addrtreepage,
        int depth, int indent);

#endif /* ADDRTREEPAGE_H */
