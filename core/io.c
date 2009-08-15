/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#include "io.h"

/*
 * The global array of object-reading functions: one for each internal state
 * specified in "object.h" and at the same index as its corresponding #define.
 */
obl_object_read_function obl_read_functions[] = {
  &obl_read_integer,
  &obl_read_boolean,
  &obl_read_slotted,
  &obl_read_shape
};

obl_object obl_read_integer(obl_shape_object shape, FILE *source) {
  obl_object o;
  return o;
}

obl_object obl_read_boolean(obl_shape_object shape, FILE *source) {
  obl_object o;
  return o;
}

obl_object obl_read_slotted(obl_shape_object shape, FILE *source) {
  obl_object o;
  return o;
}

obl_object obl_read_shape(obl_shape_object shape, FILE *source) {
  obl_object o;
  return o;
}

obl_object obl_read_object(FILE *source) {
  obl_object o;
  return o;
}
