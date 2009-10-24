/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/boolean.h"

#include "database.h"
#include "storage/object.h"

#include <stdlib.h>

struct obl_object *_obl_create_bool(struct obl_database *d, int truth)
{
    struct obl_object *result, *shape;
    struct obl_boolean_storage *storage;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_boolean_storage*)
            malloc(sizeof(struct obl_boolean_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    if (truth) {
        storage->value = (obl_uint) 1;
    } else {
        storage->value = (obl_uint) 0;
    }

    result->shape = obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR);
    result->storage.boolean_storage = storage;

    return result;
}

int obl_boolean_value(struct obl_object *bool)
{
    if (obl_storage_of(bool) != OBL_BOOLEAN) {
        OBL_WARN(bool->database, "Non-boolean object: assuming truth.");
        return 1;
    }

    return (int) (bool->storage.boolean_storage->value);
}

void obl_print_boolean(struct obl_object *boolean, int depth, int indent)
{
    int in;

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf(obl_boolean_value(boolean) ? "true" : "false");
}
