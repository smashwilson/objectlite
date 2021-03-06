/**
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
#include "session.h"

/* Internal function prototypes. */

static struct obl_object *_get_allocator(struct obl_session *s);

/* External function definitions. */

obl_logical_address obl_allocate_logical(struct obl_session *s)
{
    struct obl_object *allocator, *next_logical;
    obl_logical_address result;

    allocator = _get_allocator(s);
    if (allocator == NULL)
        return OBL_LOGICAL_UNASSIGNED;

    next_logical = obl_slotted_at(allocator, (obl_uint) 0);
    result = obl_integer_value(next_logical);
    obl_integer_set(next_logical, result + 1);

    return result;
}

obl_physical_address obl_allocate_physical(struct obl_session *s,
        obl_uint size)
{
    struct obl_object *allocator, *next_physical;
    obl_physical_address result;

    allocator = _get_allocator(s);
    if (allocator == NULL)
        return OBL_PHYSICAL_UNASSIGNED;

    next_physical = obl_slotted_at(allocator, (obl_uint) 1);
    result = obl_integer_value(next_physical);
    obl_integer_set(next_physical, result + size);

    return result;
}

static struct obl_object *_get_allocator(struct obl_session *s)
{
    struct obl_database *d = s->database;
    struct obl_object *allocator;

    allocator = obl_at_address(s, d->root.allocator_addr);

    if (allocator->shape != _obl_at_fixed_address(OBL_ALLOCATOR_SHAPE_ADDR)) {
        obl_report_error(d, OBL_MISSING_SYSTEM_OBJECT,
                "Allocator has incorrect shape.");
        return NULL;
    }

    return allocator;
}
