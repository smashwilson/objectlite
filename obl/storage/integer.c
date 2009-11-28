/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/integer.h"

#include "storage/object.h"
#include "database.h"
#include "session.h"

#include <stdlib.h>
#include <stdio.h>

struct obl_object *obl_create_integer(obl_int i)
{
    struct obl_object *result;
    struct obl_integer_storage *storage;

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }

    storage = malloc(sizeof(struct obl_integer_storage));
    if (storage == NULL) {
        obl_report_error(NULL, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }

    result->shape = _obl_at_fixed_address(OBL_INTEGER_SHAPE_ADDR);
    storage->value = i;
    result->storage.integer_storage = storage;
    return result;
}

obl_int obl_integer_value(struct obl_object *integer)
{
    if (obl_storage_of(integer) != OBL_INTEGER) {
        obl_report_error(obl_database_of(integer), OBL_WRONG_STORAGE,
                "obl_integer_value called with a non-INTEGER object.");
        return 0;
    }

    return integer->storage.integer_storage->value;
}

void obl_integer_set(struct obl_object *integer, obl_int value)
{
    if (obl_storage_of(integer) != OBL_INTEGER) {
        obl_report_error(obl_database_of(integer), OBL_WRONG_STORAGE,
                "obl_integer_set requires an object with INTEGER storage.");
        return ;
    }

    integer->storage.integer_storage->value = value;
}

/* Integers are stored in 32 bits, network byte order. */
struct obl_object *obl_integer_read(struct obl_session *session,
        struct obl_object *shape, obl_uint *source,
        obl_physical_address base, int depth)
{
    return obl_create_integer(readable_int(source[base + 1]));
}

void obl_integer_write(struct obl_object *integer, obl_uint *dest)
{
    obl_int value;

    value = obl_integer_value(integer);
    dest[integer->physical_address + 1] = writable_int(value);
}

void obl_integer_print(struct obl_object *integer, int depth, int indent)
{
    int in;

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("%d", obl_integer_value(integer));
}
