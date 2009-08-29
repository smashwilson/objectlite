/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#include <stdlib.h>

#include "io.h"
#include "platform.h"

/*
 * The global array of object-reading functions: one for each internal state
 * specified in "object.h" and at the same index as its corresponding #define.
 */
obl_object_read_function obl_read_functions[] = {
  &obl_read_shape,
  &obl_read_slotted,
  &obl_read_integer,
  &obl_read_boolean
};

obl_object *obl_read_integer(obl_shape_object *shape, FILE *source) {
  obl_integer_object raw;
  obl_object *o;
  obl_integer_object *internal;

  o = malloc(sizeof(obl_object));
  if( o == NULL ) {
    /* TODO: set error message here. */
    return NULL;
  }
  o->shape = shape;
  o->internal_format = OBL_INTERNAL_INTEGER;

  if( fread(&raw, sizeof(obl_integer_object), 1, source) != 1 ) {
    /* TODO: set error message here. */
    return NULL;
  }

  internal = malloc(sizeof(obl_integer_object));
  if( internal == NULL ) {
    /* TODO: set error message here. */
    return NULL;
  }
  *internal = (obl_integer_object) ntohl(raw);
  o->internal_storage.integer = internal;

  return o;
}

obl_object *obl_read_boolean(obl_shape_object *shape, FILE *source) {
  return NULL;
}

obl_object *obl_read_slotted(obl_shape_object *shape, FILE *source) {
  return NULL;
}

obl_object *obl_read_shape(obl_shape_object *shape, FILE *source) {
  return NULL;
}

obl_object *obl_read_object(FILE *source) {
  return NULL;
}
