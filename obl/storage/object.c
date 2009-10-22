/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in
 * ObjectLite.
 */

#include "storage/object.h"

#include "database.h"

#include <stdlib.h>
#include <stdio.h>

#include "unicode/ucnv.h"

obl_uint obl_object_wordsize(struct obl_object *o)
{
    switch(_obl_storage_of(o)) {
    case OBL_SHAPE:
        return 5;
    case OBL_SLOTTED:
        return 1 + obl_shape_slotcount(o->shape);
    case OBL_FIXED:
        return 1 + obl_fixed_size(o);
    case OBL_CHUNK:
        return 2 + CHUNK_SIZE;
    case OBL_ADDRTREEPAGE:
        return 2 + CHUNK_SIZE;
    case OBL_INTEGER:
        return 2;
    case OBL_FLOAT:
        return 1 + ceil((double) 32 / sizeof(obl_uint));
    case OBL_DOUBLE:
        return 1 + ceil((double) 64 / sizeof(obl_uint));
    case OBL_CHAR:
        return 1 + ceil((double) sizeof(obl_uint) / sizeof(UChar32));
    case OBL_STRING:
        return 1 + obl_string_size(o) *
            ceil((double) sizeof(obl_uint) / sizeof(UChar));
    case OBL_BOOLEAN:
        return 2;
    case OBL_NIL:
        return 2;
    default:
        obl_report_error(o->database, OBL_WRONG_STORAGE,
                "obl_object_wordsize called with an object of unknown storage.");
        return 0;
    }
}

void obl_destroy_object(struct obl_object *o)
{
    if (o->storage.any_storage != NULL ) {
        free(o->storage.any_storage);
    }
    free(o);
}

enum obl_storage_type _obl_storage_of(const struct obl_object *o)
{
    if (o->shape == obl_nil(o->database)) {
        return OBL_SHAPE;
    } else {
        return (enum obl_storage_type)
                o->shape->storage.shape_storage->storage_format;
    }
}

struct obl_object *_obl_allocate_object(struct obl_database *d)
{
    struct obl_object *result = (struct obl_object *)
            malloc(sizeof(struct obl_object));

    if (result == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    result->database = d;
    result->logical_address = OBL_LOGICAL_UNASSIGNED;
    result->physical_address = OBL_PHYSICAL_UNASSIGNED;
    return result;
}
