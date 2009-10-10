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

/*
 * The global array of object-reading functions: one for each internal state
 * specified in "object.h" and at the same index as its corresponding #define.
 */
obl_object_read_function obl_read_functions[] = {
        &obl_read_shape,
        &obl_read_slotted, &obl_read_integer, &obl_read_boolean
};

struct obl_object *obl_read_integer(struct obl_object *shape, FILE *source)
{
    return NULL;
}

struct obl_object *obl_read_boolean(struct obl_object *shape, FILE *source)
{
    return NULL;
}

struct obl_object *obl_read_slotted(struct obl_object *shape, FILE *source)
{
    return NULL;
}

struct obl_object *obl_read_shape(struct obl_object *shape, FILE *source)
{
    return NULL;
}

struct obl_object *obl_read_object(FILE *source)
{
    return NULL;
}
