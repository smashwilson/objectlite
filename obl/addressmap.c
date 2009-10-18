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

#include "allocator.h"
#include "constants.h"
#include "database.h"
#include "object.h"

/* Prototypes for internal functions */

/*
 * Returns 1 if the structure at physical address +base+ is an address map tree
 * page.  Sets an error message and returns 0 if it is not.
 */
static int _verify_addrtreepage(struct obl_database *d,
        obl_physical_address base);

/* Traverses the address map tree to find the entry for +logical+. */
static obl_physical_address _lookup_in(struct obl_database *d,
        obl_physical_address pagebase, obl_logical_address logical);

/*
 * Traverse the address map and assign +value+ to +key+ in the appropriate
 * page.
 */
static void _assign_in(struct obl_database *d,
        obl_physical_address pagebase,
        obl_logical_address key, obl_physical_address value);

/* Isolates the height-th PAGE_SHIFT bits out of address. */
static inline obl_uint _treepage_index(obl_logical_address logical,
        obl_uint height);

/*
 * A few #defines useful for the bit logic that don't really belong in
 * constants.h.
 */

#define CHUNK_MASK (CHUNK_SIZE - 1)
#define PAGE_SHIFT (CHUNK_SIZE_LOG2 - 1)

obl_physical_address obl_address_lookup(struct obl_database *d,
        obl_logical_address logical)
{
    obl_uint base, height, mask;

    base = d->root.address_map_addr;
    if (! _verify_addrtreepage(d, base)) {
        return OBL_PHYSICAL_UNASSIGNED;
    }

    height = readable_uint(d->content[base + 1]);
    mask = ~((1 << (PAGE_SHIFT * (height + 1))) - 1);

    if (mask & (obl_uint) logical != 0) {
        return OBL_PHYSICAL_UNASSIGNED;
    }

    return _lookup_in(d, base, logical);
}

void obl_address_assign(struct obl_database *d,
        obl_logical_address logical, obl_physical_address physical)
{
    obl_physical_address base;
    obl_uint height, copied_logical, required_height;

    copied_logical = (obl_uint) logical;
    required_height = (obl_uint) 0;
    while (copied_logical >>= PAGE_SHIFT) {
        required_height++;
    }

    base = d->root.address_map_addr;
    if (! _verify_addrtreepage(d, base)) {
        return ;
    }

    height = readable_uint(d->content[base + 1]);
    if (height < required_height) {
        /* TODO allocate required_height - height new tree pages. */
        /* TODO update the address map pointer to the new tree root. */
        return ;
    }

    _assign_in(d, base, logical, physical);
}

/*
 * Internal functions.
 */

static obl_physical_address _lookup_in(struct obl_database *d,
        obl_physical_address pagebase, obl_logical_address logical)
{
    obl_uint height, index;
    obl_physical_address value;

    if (! _verify_addrtreepage(d, pagebase)) {
        return OBL_PHYSICAL_UNASSIGNED;
    }

    height = readable_uint(d->content[pagebase + 1]);
    index = _treepage_index(logical, height);

    value = (obl_physical_address) readable_uint(
            d->content[pagebase + 2 + index]);

    if (height == 0 || value == OBL_PHYSICAL_UNASSIGNED) {
        return value;
    } else {
        return _lookup_in(d, (obl_physical_address) value, logical);
    }
}

static void _assign_in(struct obl_database *d,
        obl_physical_address pagebase,
        obl_logical_address key, obl_physical_address value)
{
    obl_uint height, index;

    if (! _verify_addrtreepage(d, pagebase)) {
        return ;
    }

    height = readable_uint(d->content[pagebase + 1]);
    index = _treepage_index(key, height);

    if (height == 0) {
        d->content[pagebase + 2 + index] = writable_uint((obl_uint) value);
    } else {
        obl_uint next_page;

        next_page = readable_uint(d->content[pagebase + 2 + index]);
        _assign_in(d, (obl_physical_address) next_page, key, value);
    }
}

static int _verify_addrtreepage(struct obl_database *d,
        obl_physical_address base)
{
    obl_uint storage_addr;

    storage_addr = readable_uint(d->content[base]);
    if (storage_addr != (obl_uint) OBL_ADDRTREEPAGE_SHAPE_ADDR) {
        obl_report_error(d, OBL_WRONG_STORAGE,
                "The address map is corrupted.");
        return 0;
    } else {
        return 1;
    }
}

static inline obl_uint _treepage_index(obl_logical_address logical,
        obl_uint height)
{
    obl_uint shift;

    shift = PAGE_SHIFT * height;
    return (obl_uint) (logical & ((CHUNK_SIZE - 1) << shift)) >> shift;
}
