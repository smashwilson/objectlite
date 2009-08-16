/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#ifndef IO_H
#define IO_H

#include "object.h"

#include <stdio.h>

/*
 * Signature of a function that reads and populates an object's internal state.
 * Each such function is provided the shape determined from the shape word and a
 * FILE* positioned immediately after the shape word.
 */
typedef obl_object *(*obl_object_read_function)(obl_shape_object *shape, FILE *source);

/* Read a single-word obl_integer_object. */
obl_object *obl_read_integer(obl_shape_object *shape, FILE *source);

/* Read a single-word obl_boolean_object. */
obl_object *obl_read_boolean(obl_shape_object *shape, FILE *source);

/*
 * Read a slotted object.  The number of slots expected is determined by the
 * provided shape.
 */
obl_object *obl_read_slotted(obl_shape_object *shape, FILE *source);

/*
 * Read a shape object.  Shapes are themselves a fixed shape (sorry, no turtles
 * all the way down -- yet).
 */
obl_object *obl_read_shape(obl_shape_object *shape, FILE *source);

/*
 * Read a shape word from the current position of the file <source>, retrieve
 * the shape object, then invoke the appropriate obl_object_read_function from
 * the <obl_read_functions> table to read the rest of the object.  Return the
 * populated obl_object structure.
 */
obl_object *obl_read_object(FILE *source);

#endif
