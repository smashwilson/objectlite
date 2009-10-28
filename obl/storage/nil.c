/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/nil.h"

#include "storage/object.h"
#include "database.h"

struct obl_object *_obl_create_nil(struct obl_database *d)
{
    struct obl_object *result, *shape;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    result->shape = obl_at_address(d, OBL_NIL_SHAPE_ADDR);
    result->storage.nil_storage = NULL;

    return result;
}

void obl_nil_print(struct obl_object *nil, int depth, int indent)
{
    int in;

    for (in = 0; in < indent; in++) { putchar(' '); }
    puts("nil");
}
