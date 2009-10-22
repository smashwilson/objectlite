/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/fixed.h"

#include "storage/object.h"
#include "database.h"

#include <stdio.h>

struct obl_object *obl_create_fixed(struct obl_database *d, obl_uint length)
{
    struct obl_object *result;
    struct obl_fixed_storage *storage;
    obl_uint i;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_fixed_storage *)
            malloc(sizeof(struct obl_fixed_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.fixed_storage = storage;
    result->shape = obl_at_address(d, OBL_FIXED_SHAPE_ADDR);

    storage->length = length;
    storage->contents = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * length);
    if (storage->contents == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        free(storage);
        return NULL;
    }

    for (i = 0; i < length; i++) {
        storage->contents[i] = obl_nil(d);
    }

    return result;
}

obl_uint obl_fixed_size(struct obl_object *fixed)
{
    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                "obl_fixed_size requires an object with FIXED storage.");
        return 0;
    }

    return fixed->storage.fixed_storage->length;
}

struct obl_object *obl_fixed_at(struct obl_object *fixed, obl_uint index)
{
    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return obl_nil(fixed->database);
    }

    if (index >= obl_fixed_size(fixed)) {
        obl_report_errorf(fixed->database, OBL_INVALID_INDEX,
                "obl_fixed_at called with an invalid index (%d, valid 0..%d)",
                index,
                obl_fixed_size(fixed) - 1);
        return obl_nil(fixed->database);
    }

    return _obl_resolve_stub(fixed->storage.fixed_storage->contents[index]);
}

void obl_fixed_at_put(struct obl_object *fixed, const obl_uint index,
        struct obl_object *value)
{
    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return ;
    }

    if (index >= obl_fixed_size(fixed)) {
        obl_report_errorf(fixed->database, OBL_INVALID_INDEX,
                "obl_fixed_at_put called with an invalid index (%d, valid 0..%d)",
                index,
                obl_fixed_size(fixed) - 1);
        return ;
    }

    fixed->storage.fixed_storage->contents[index] = value;
}

void obl_print_fixed(struct obl_object *fixed, int depth, int indent)
{
    int ind, i;

    for (ind = 0; ind < indent; ind++) { putchar(' '); }
    if (depth == 0) {
        printf("<fixed collection: %d elements>\n",
                obl_fixed_size(fixed));
        return ;
    }
    puts("Fixed Collection");

    for (i = 0; i < obl_fixed_size(fixed); i++) {
        obl_print_object(obl_fixed_at(fixed, i), depth - 1, indent + 2);
        printf("\n");
    }
}
