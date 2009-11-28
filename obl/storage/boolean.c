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
#include <stdio.h>

struct obl_object *_obl_create_bool(int truth)
{
    struct obl_object *result;
    struct obl_boolean_storage *storage;

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }

    storage = malloc(sizeof(struct obl_boolean_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(NULL, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    if (truth) {
        storage->value = (obl_uint) 1;
    } else {
        storage->value = (obl_uint) 0;
    }

    result->shape = _obl_at_fixed_address(OBL_BOOLEAN_SHAPE_ADDR);
    result->storage.boolean_storage = storage;

    return result;
}

int obl_boolean_value(struct obl_object *bool)
{
    if (obl_storage_of(bool) != OBL_BOOLEAN) {
        OBL_WARN(obl_database_of(bool), "Non-boolean object: assuming truth.");
        return 1;
    }

    return (int) (bool->storage.boolean_storage->value);
}

void obl_boolean_print(struct obl_object *boolean, int depth, int indent)
{
    int in;

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf(obl_boolean_value(boolean) ? "true" : "false");
}
