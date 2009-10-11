/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#include "io.h"

#include "database.h"
#include "object.h"
#include "platform.h"

#include <stdlib.h>

    /* OBL_SHAPE,
    OBL_SLOTTED, OBL_FIXED, OBL_CHUNK, OBL_TREEPAGE,
    OBL_INTEGER, OBL_FLOAT, OBL_DOUBLE, OBL_CHAR, OBL_STRING, OBL_BOOLEAN,
    OBL_NIL, OBL_STUB, */

/*
 * The array of object-reading functions: one for each internal state
 * specified in "object.h" and at the same index as its index in the
 * +obl_storage_type+ enumeration.
 */
obl_object_read_function obl_read_functions[] = {
        &obl_read_shape,      /* OBL_SHAPE */
        &obl_read_slotted,    /* OBL_SLOTTED */
        &obl_invalid_storage, /* OBL_FIXED */
        &obl_invalid_storage, /* OBL_CHUNK */
        &obl_invalid_storage, /* OBL_TREEPAGE */
        &obl_read_integer,    /* OBL_INTEGER */
        &obl_invalid_storage, /* OBL_FLOAT */
        &obl_invalid_storage, /* OBL_DOUBLE */
        &obl_invalid_storage, /* OBL_CHAR */
        &obl_invalid_storage, /* OBL_STRING */
        &obl_invalid_storage, /* OBL_BOOLEAN (invalid) */
        &obl_invalid_storage, /* OBL_NIL (invalid) */
        &obl_invalid_storage  /* OBL_STUB (invalid) */
};

/* Integers are stored in 32 bits, network byte order. */
struct obl_object *obl_read_integer(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset)
{
    obl_int value;
    struct obl_object *o;

    value = readable_int(source[offset]);
    o = obl_create_integer(shape->database, value);
    o->physical_address = offset;

    return o;
}

struct obl_object *obl_read_slotted(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset)
{
    return obl_nil(shape->database);
}

struct obl_object *obl_read_shape(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset)
{
    return obl_nil(shape->database);
}

struct obl_object *obl_invalid_storage(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset)
{
    obl_report_errorf(shape->database, OBL_WRONG_STORAGE,
            "Attempt to read an object (%u) with an invalid storage type.",
            offset);
    return obl_nil(shape->database);
}

struct obl_object *obl_read_object(struct obl_database *d,
        obl_uint *source, obl_physical_address offset)
{
    return NULL;
}
