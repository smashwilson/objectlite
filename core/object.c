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

/*
 * ============================================================================
 * Prototypes for internal functions.
 * ============================================================================
 */

inline obl_storage_type _storage_of(const obl_object *o);

inline obl_object *_allocate_object();

/*
 * ============================================================================
 * Object creation functions.
 * ============================================================================
 */

obl_object *obl_create_integer(obl_database *d, int i)
{
    obl_object *result;
    obl_integer_storage *storage;

    if (i < INT32_MIN || i > INT32_MAX) {
        obl_report_error(d, CONVERSION_ERROR, "Integer out of range.");
        return NULL;
    }

    result = _allocate_object();
    if (result == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        return NULL;
    }

    storage = (obl_integer_storage *) malloc(sizeof(obl_integer_storage));
    if (storage == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
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

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_error(d, OUT_OF_MEMORY,
                "Unable to allocate Unicode converter.");
        return NULL;
    }

    output_length = length * 2;
    output_string = (UChar *) malloc(sizeof(UChar) * output_length);
    if (output_string == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate Unicode string.");
        return NULL;
    }

    converted_length = ucnv_toUChars(converter, output_string, output_length,
            c, length, &status);
    ucnv_close(converter);
    if (U_FAILURE(status)) {
        obl_report_error(d, CONVERSION_ERROR, "Unicode conversion failure.");
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
    if (result == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        return NULL;
    }
    storage = (obl_string_storage *) malloc(sizeof(obl_string_storage));
    if (storage == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        free(result);
        return NULL;
    }
    result->storage.string_storage = storage;
    result->shape = obl_at_address(d, OBL_STRING_SHAPE_ADDR);

    storage->length = (uint32_t) length;
    storage->contents = uc;

    return result;
}

obl_object *obl_create_fixed(obl_database *d, uint32_t length)
{
    obl_object *result;
    obl_fixed_storage *storage;
    uint32_t i;

    result = _allocate_object();
    if (result == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        return NULL;
    }

    storage = (obl_fixed_storage *) malloc(sizeof(obl_fixed_storage));
    if (storage == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        free(result);
        return NULL;
    }
    result->storage.fixed_storage = storage;
    result->shape = obl_at_address(d, OBL_FIXED_SHAPE_ADDR);

    storage->length = length;
    storage->contents = (obl_object **) malloc(sizeof(obl_object*) * length);
    if (storage->contents == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        free(result);
        free(storage);
        return NULL;
    }

    for (i = 0; i < length; i++) {
        storage->contents[i] = obl_at_address(d, OBL_NIL_ADDR);
    }

    return result;
}

obl_object *obl_create_stub(obl_database *d, obl_logical_address address)
{
    return NULL;
}

obl_object *obl_create_shape(obl_database *d, char *name, char **slot_names,
        obl_storage_type type)
{
    return NULL;
}

/*
 * ============================================================================
 * Common operations on shapes and built-in collection types.
 * ============================================================================
 */

uint32_t obl_fixed_size(obl_object *o)
{
    /* if (_storage_of(o) != OBL_FIXED_STORAGE) {
     return 0;
     } */

    return o->storage.fixed_storage->length;
}

obl_object *obl_fixed_at(obl_object *o, const uint32_t index)
{
    /* if (_storage_of(o) != OBL_FIXED_STORAGE) {
     return 0;
     } */

    return o->storage.fixed_storage->contents[index];
}

void obl_fixed_at_put(obl_object *o, const uint32_t index, obl_object *value)
{
    /* if (_storage_of(o) != OBL_FIXED_STORAGE) {
     return 0;
     } */

    o->storage.fixed_storage->contents[index] = value;
}

/*
 * ============================================================================
 * Functions to translate obl_object values into primitive C types.
 * ============================================================================
 */

int obl_integer_value(const obl_object *o)
{
    /* if (_storage_of(o) != OBL_INTEGER) {
     return 0;
     } */

    return (int) *(o->storage.integer_storage);
}

size_t obl_string_size(const obl_object *o)
{
    /* if (_storage_of(o) != OBL_STRING) {
     return 0;
     } */

    return o->storage.string_storage->length;
}

size_t obl_string_value(const obl_object *o, UChar *buffer, size_t buffer_size)
{
    obl_string_storage *storage;
    size_t count;

    /* if (_storage_of(o) != OBL_STRING) {
     return 0;
     } */

    storage = o->storage.string_storage;
    count = storage->length > buffer_size ? storage->length : buffer_size;

    memcpy(buffer, o->storage.string_storage->contents, count);

    return count;
}

size_t obl_string_chars(const obl_object *o, char *buffer, size_t buffer_size)
{
    obl_string_storage *storage;
    UConverter *converter;
    uint32_t converted_length;
    UErrorCode status = U_ZERO_ERROR;

    /* if (_storage_of(o) != OBL_STRING) {
     return 0;
     } */

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_error(o->database, OUT_OF_MEMORY,
                "Unable to allocate Unicode converter.");
        return 0;
    }

    storage = o->storage.string_storage;

    converted_length = ucnv_fromUChars(converter, buffer, buffer_size,
            storage->contents, storage->length, &status);
    ucnv_close(converter);

    if (U_FAILURE(status)) {
        obl_report_error(o->database, CONVERSION_ERROR,
                "Unable to convert string from UTF-16.");
    }

    return converted_length;
}

/*
 * ============================================================================
 * Orderly object destruction.
 * ============================================================================
 */

void obl_destroy_object(obl_object *o)
{
    switch (_storage_of(o)) {
    case OBL_INTEGER:
        free(o->storage.integer_storage);
        break;
    case OBL_STRING:
        free(o->storage.string_storage->contents);
        break;
    }

    free(o);
}

/*
 * ============================================================================
 * Internal function definitions.
 * ============================================================================
 */

/* Retrieve the internal storage type of an object from its assigned shape. */
inline obl_storage_type _storage_of(const obl_object *o)
{
    if (o->shape == NULL) {
        return OBL_SHAPE;
    } else {
        return (obl_storage_type) obl_integer_value(
                o->shape->storage.shape_storage->storage_format);
    }
}

/* Allocate and perform common initialization for an unpersisted obl_object. */
inline obl_object *_allocate_object()
{
    obl_object *result = (obl_object *) malloc(sizeof(obl_object));
    result->database = NULL;
    result->logical_address = 0;
    result->physical_address = 0;
    return result;
}
