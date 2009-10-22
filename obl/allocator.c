/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * The simplest possible allocator implementation.  It uses a one-up counter
 * for logical addresses, and increments physical addresses by object size.
 */

#include "allocator.h"

#include "storage/object.h"
#include "database.h"

obl_logical_address obl_allocate_logical(struct obl_database *d)
{
    struct obl_object *allocator, *next_logical;
    obl_logical_address result;

    allocator = obl_at_address_depth(d, d->root.allocator_addr, 2);

    if (allocator->shape != obl_at_address(d, OBL_ALLOCATOR_SHAPE_ADDR)) {
        obl_report_error(d, OBL_MISSING_SYSTEM_OBJECT,
                "Allocator has incorrect shape.");
        return OBL_LOGICAL_UNASSIGNED;
    }

    next_logical = obl_slotted_at(allocator, (obl_uint) 0);
    result = obl_integer_value(next_logical);
    obl_integer_set(next_logical, result + 1);

    return result;
}

obl_physical_address obl_allocate_physical(struct obl_database *d,
        obl_uint size)
{
    struct obl_object *allocator, *next_physical;
    obl_physical_address result;

    allocator = obl_at_address_depth(d, d->root.allocator_addr, 2);

    if (allocator->shape != obl_at_address(d, OBL_ALLOCATOR_SHAPE_ADDR)) {
        obl_report_error(d, OBL_MISSING_SYSTEM_OBJECT,
                "Allocator has incorrect shape.");
        return OBL_LOGICAL_UNASSIGNED;
    }

    next_physical = obl_slotted_at(allocator, (obl_uint) 1);
    result = obl_integer_value(next_physical);
    obl_integer_set(next_physical, result + size);

    return result;
}
