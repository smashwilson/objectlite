/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implementation of the logical-to-physical address map service within
 * ObjectLite.  Uses a B+ tree of obl_objects with OBL_ADDRTREEPAGE storage.
 *
 * Both for efficiency and to avoid circular dependencies, addressmap.c does
 * its own file I/O.  In particular, the mapping of new addresses directly
 * performs database writes.
 */

#include "addressmap.h"

#include "constants.h"
#include "database.h"
#include "object.h"

/* Prototypes for internal functions */

static obl_physical_address address_for(struct obl_database *d,
        obl_physical_address pagebase, obl_logical_address logical);

/*
 * A few #defines useful for the bit logic that don't really belong in
 * constants.h.
 */

#define CHUNK_MASK (CHUNK_SIZE - 1)
#define PAGE_SHIFT (CHUNK_SIZE_LOG2 - 1)

obl_physical_address obl_address_for(struct obl_database *d,
        obl_logical_address logical)
{
    obl_uint base;
    obl_uint storageaddr, height;
    obl_uint mask;

    base = d->root.address_map;
    storageaddr = readable_uint(d->content[base]);

    if (storageaddr != (obl_uint) OBL_ADDRTREEPAGE_SHAPE_ADDR) {
        obl_report_error(d, OBL_WRONG_STORAGE,
                "Missing address map.");
        return OBL_PHYSICAL_UNASSIGNED;
    }

    height = readable_uint(d->content[base + 1]);
    mask = ~((1 << (PAGE_SHIFT * (height + 1))) - 1);

    if (mask & (obl_uint) logical != 0) {
        return OBL_PHYSICAL_UNASSIGNED;
    }

    return address_for(d, base, logical);
}

/*
 * Internal functions.
 */

static obl_physical_address address_for(struct obl_database *d,
        obl_physical_address pagebase, obl_logical_address logical)
{
    obl_uint storageaddr, height;
    obl_uint shift, index;
    obl_physical_address value;

    storageaddr = readable_uint(d->content[pagebase]);

    if (storageaddr != (obl_uint) OBL_ADDRTREEPAGE_SHAPE_ADDR) {
        obl_report_error(d, OBL_WRONG_STORAGE,
                "Address map is corrupted.");
        return OBL_PHYSICAL_UNASSIGNED;
    }

    height = readable_uint(d->content[pagebase + 1]);

    /* Isolates the height-th PAGE_SHIFT bits out of address. */
    shift = PAGE_SHIFT * height;
    index = (logical & ((CHUNK_SIZE - 1) << shift)) >> shift;

    value = (obl_physical_address) readable_uint(
            d->content[pagebase + 2 + index]);

    if (height == 0 || value == OBL_PHYSICAL_UNASSIGNED) {
        return value;
    } else {
        return address_for(d, (obl_physical_address) value, logical);
    }
}
