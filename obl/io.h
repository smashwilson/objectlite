/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#ifndef IO_H
#define IO_H

#include <stdio.h>

#include "platform.h"
#include "object.h"

/*
 * Signature of a function that reads and populates an object's internal state.
 * Each such function is provided the shape determined from the shape word and a
 * FILE* positioned immediately after the shape word.
 */
typedef struct obl_object *(*obl_object_read_function)(
        struct obl_object *shape, FILE *source);

/* Read a single-word obl_integer_object. */
struct obl_object *obl_read_integer(struct obl_object *shape, FILE *source);

/* Read a single-word obl_boolean_object. */
struct obl_object *obl_read_boolean(struct obl_object *shape, FILE *source);

/*
 * Read a slotted object.  The number of slots expected is determined by the
 * provided shape.
 */
struct obl_object *obl_read_slotted(struct obl_object *shape, FILE *source);

/*
 * Read a shape object.  Shapes are themselves a fixed shape (sorry, no turtles
 * all the way down -- yet).
 */
struct obl_object *obl_read_shape(struct obl_object *shape, FILE *source);

/*
 * Read a shape word from the current position of the file <source>, retrieve
 * the shape object, then invoke the appropriate struct obl_object_read_function from
 * the <obl_read_functions> table to read the rest of the object.  Return the
 * populated struct obl_object structure.
 *
 * The created object lives on the heap and must be destroyed with a call to
 * obl_destroy_object() as defined in "object.h".
 */
struct obl_object *obl_read_object(FILE *source);

#endif
