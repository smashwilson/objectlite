/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in ObjectLite.
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

/* 
 * Used to specify the logical address of a remote object that's stored
 * elsewhere in the database.
 */
typedef uint32_t obl_object_address;

/*
 * Shape object: a Class object which specifies how to interpret any object
 * whose header word points to it.  Most significant is the storage format, an
 * index into the global obl_read_functions array defined in "io.h".
 */
#define OBL_INTERNAL_SHAPE 0
typedef struct {
  obl_object_address name;
  obl_object_address slot_names;
  obl_object_address current_shape;
  uint32_t storage_format;
} obl_shape_object;

/*
 * Slotted object: an object that contains zero to many "slots" (more commonly
 * known as instance variables).  Each slot contains a reference to another
 * object.  The number of slots and the names of each slot are specified within
 * the object's shape.
 */
#define OBL_INTERNAL_SLOTTED 1
typedef struct {
  obl_object_address *slots;
} obl_slotted_object;

/*
 * Integer object: a signed integer value within the range +/-2^31 - 1.
 */
#define OBL_INTERNAL_INTEGER 2
typedef int32_t obl_integer_object;

/*
 * Boolean object: truth or falsehood.
 */
#define OBL_INTERNAL_BOOLEAN 3
typedef uint32_t obl_boolean_object;

/*
 * A wrapper that contains an object's shape and internal storage.  Most
 * external and language binding code should work with obl_object objects
 * and use the functions provided here to manipulate them.
 */
typedef struct {
  obl_shape_object *shape;
  uint8_t internal_format;
  union {
    obl_integer_object *integer;
    obl_boolean_object *boolean;
    obl_slotted_object *slotted;
  } internal_storage;
} obl_object;

void obl_destroy_object(obl_object *o);

#endif
