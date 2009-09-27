/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in ObjectLite.
 */

#include "object.h"

#include <stdlib.h>

#include "unicode/ucnv.h"

/* Prototypes for internal functions. */

inline obl_storage_type _storage_of(const obl_object *o);

inline obl_object *_allocate_object();

/* Object creation functions. */

obl_object *obl_create_integer(obl_database *d, int i)
{
  obl_object *result;
  obl_integer_storage *storage;

  if( i < INT32_MIN || i > INT32_MAX ) {
    obl_report_error(d, CONVERSION_ERROR,
                     "Integer out of range.");
    return NULL;
  }

  result = _allocate_object();
  if( result == NULL ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate a new object.");
    return NULL;
  }

  storage = (obl_integer_storage *) malloc(sizeof(obl_integer_storage));
  if( storage == NULL ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate a new object.");
    free(result);
    return NULL;
  }

  result->shape = obl_at_address(d, OBL_INTEGER_SHAPE_ADDR);
  *storage = (obl_integer_storage) i;
  result->storage.integer_storage = storage;
  return result;
}

obl_object *obl_create_float(obl_database *d, float f)
{
  return NULL;
}

obl_object *obl_create_double(obl_database *d, double dbl)
{
  return NULL;
}

obl_object *obl_create_char(obl_database *d, char c)
{
  return NULL;
}

obl_object *obl_create_uchar(obl_database *d, UChar32 uc)
{
  return NULL;
}

obl_object *obl_create_string(obl_database *d, char *c, int32_t length)
{
  UConverter *converter;
  int32_t output_length;
  UChar *output_string;
  int32_t converted_length;
  UErrorCode status = U_ZERO_ERROR;

  converter = ucnv_open("US-ASCII", &status);
  if( U_FAILURE(status) ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate Unicode converter.");
    return NULL;
  }

  output_length = strlen(c) * 2;
  output_string = (UChar *) malloc( sizeof(UChar) * output_length );
  if( output_string == NULL ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate Unicode string.");
    return NULL;
  }

  converted_length = ucnv_toUChars(converter,
                                   output_string, output_length,
                                   c, length,
                                   &status);
  ucnv_close(converter);
  if( U_FAILURE(status) ) {
    obl_report_error(d, CONVERSION_ERROR,
                     "Unicode conversion failure.");
    free(output_string);
    return NULL;
  }

  return obl_create_ustring(d, output_string, converted_length);
}

obl_object *obl_create_ustring(obl_database *d, UChar *uc, int32_t length)
{
  obl_object *result;
  obl_string_storage *storage;

  result = _allocate_object();
  if( result == NULL ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate a new object.");
    return NULL;
  }
  storage = (obl_string_storage *) malloc( sizeof(obl_string_storage) );
  if( storage == NULL ) {
    obl_report_error(d, OUT_OF_MEMORY,
                     "Unable to allocate a new object.");
    free(result);
    return NULL;
  }
  result->shape = obl_at_address(d, OBL_STRING_SHAPE_ADDR);

  storage->length = (uint32_t) length;
  storage->contents = uc;

  return result;
}

obl_object *obl_create_stub(obl_database *d, obl_logical_address address)
{
  return NULL;
}

obl_object *obl_create_shape(obl_database *d,
                             char *name, char **slot_names,
                             obl_storage_type type)
{
  return NULL;
}

/* Functions to translate obl_object values into primitive C types. */

int obl_integer_value(const obl_object *o)
{
  if(_storage_of(o) != OBL_INTEGER) {
    return 0;
  }

  return (int) o->storage.integer_storage;
}

char *obl_string_value(const obl_object *o)
{
  UConverter *converter;
  int32_t output_length;
  char *output_string;
  int32_t converted_length;
  UErrorCode status = U_ZERO_ERROR;

  if(_storage_of(o) != OBL_STRING) {
    return NULL;
  }

  return NULL;
}

/* Orderly object destruction. */

void obl_destroy_object(obl_object *o)
{
  switch(_storage_of(o)) {
  case OBL_INTEGER:
    free(o->storage.integer_storage);
    break;
  case OBL_STRING:
    free(o->storage.string_storage->contents);
    break;
  }

  free(o);
}

/* Internal function definitions. */

/* Retrieve the internal storage type of an object from its assigned shape. */
inline obl_storage_type _storage_of(const obl_object *o)
{
  if( o->shape == NULL ) {
    return OBL_SHAPE;
  } else {
    return (obl_storage_type) o->shape->storage.shape_storage->storage_format;
  }
}

/* Allocate and perform common initialization for an unpersisted obl_object. */
inline obl_object *_allocate_object()
{
  obl_object *result = (obl_object *)
    malloc(sizeof(obl_object));
  result->database = NULL;
  result->logical_address = 0;
  result->physical_address = 0;
  return result;
}
