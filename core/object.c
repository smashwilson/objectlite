/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in ObjectLite.
 */

#include <stdlib.h>

#include "object.h"

/* Prototypes for internal functions. */

inline obl_storage_type storage_of(const obl_object *o);

/* Object creation functions. */

obl_object *obl_create_integer(const int i)
{
  return NULL;
}

obl_object *obl_create_float(const float f)
{
  return NULL;
}

obl_object *obl_create_double(const double d)
{
  return NULL;
}

obl_object *obl_create_char(const char c)
{
  return NULL;
}

obl_object *obl_create_uchar(const UChar32 uc)
{
  return NULL;
}

obl_object *obl_create_string(const char *c)
{
  return NULL;
}

obl_object *obl_create_ustring(const UChar *uc)
{
  return NULL;
}

obl_object *obl_create_stub(const obl_logical_address address)
{
  return NULL;
}

obl_object *obl_create_shape(const char *name, const char **slot_names,
                             const obl_storage_type type)
{
  return NULL;
}

void obl_destroy(obl_object *o)
{
}

/* Internal function definitions. */

/* Retrieve the internal storage type of an object from its assigned shape. */
inline obl_storage_type storage_of(const obl_object *o)
{
  if( o->shape == NULL ) {
    return OBL_SHAPE;
  } else {
    return (obl_storage_type) o->shape->internal_storage.shape_storage->storage_format;
  }
}
