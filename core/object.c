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

static inline obl_storage_type _storage_of(const obl_object *o);

static inline obl_object *_allocate_object(obl_database *d);

static obl_object *_allocate_string(obl_database *d, UChar *uc,
        obl_uword length);

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

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (obl_integer_storage *) malloc(sizeof(obl_integer_storage));
    if (storage == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        free(result);
        return NULL;
    }

    result->shape = obl_at_address(d, OBL_INTEGER_SHAPE_ADDR);
    storage->value = (obl_word) i;
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

obl_object *obl_create_string(obl_database *d, const UChar *uc, obl_uword length)
{
    UChar *copied;
    size_t bytes;

    bytes = sizeof(UChar) * (size_t) length;
    copied = (UChar *) malloc(bytes);
    if( copied == NULL ) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate string storage.");
        return NULL;
    }

    memcpy(copied, uc, bytes);

    return _allocate_string(d, copied, length);
}

obl_object *obl_create_cstring(obl_database *d, const char *c, obl_uword length)
{
    UConverter *converter;
    size_t output_length;
    UChar *output_string;
    size_t converted_length;
    UErrorCode status = U_ZERO_ERROR;

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_error(d, OUT_OF_MEMORY,
                "Unable to allocate Unicode converter.");
        return NULL;
    }

    output_length = (size_t) length * 2;
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

    return _allocate_string(d, output_string, (obl_uword) converted_length);
}

obl_object *obl_create_fixed(obl_database *d, obl_uword length)
{
    obl_object *result;
    obl_fixed_storage *storage;
    obl_uword i;

    result = _allocate_object(d);
    if (result == NULL) {
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
        storage->contents[i] = obl_nil(d);
    }

    return result;
}

obl_object *obl_create_slotted(obl_object *shape)
{
    obl_object *result;
    obl_slotted_storage *storage;
    obl_uword slot_count;
    obl_object **slots;
    obl_uword i;

    result = _allocate_object(shape->database);
    if (result == NULL) {
        return NULL;
    }

    storage = (obl_slotted_storage*) malloc(sizeof(obl_slotted_storage));
    if (storage == NULL) {
        obl_report_error(shape->database, OUT_OF_MEMORY,
                "Unable to allocate an object.");
        free(result);
        return NULL;
    }
    result->storage.slotted_storage = storage;
    result->shape = shape;

    slot_count = obl_shape_slotcount(shape);
    slots = (obl_object **) malloc(sizeof(obl_object*) * slot_count);
    if (slots == NULL) {
        obl_report_error(shape->database, OUT_OF_MEMORY,
                "Unable to allocate an object.");
        free(result);
        free(storage);
        return NULL;
    }
    storage->slots = slots;

    for (i = 0; i < slot_count; i++) {
        slots[i] = obl_nil(shape->database);
    }

    return result;
}

obl_object *obl_create_shape(obl_database *d,
        obl_object *name, obl_object *slot_names,
        obl_storage_type type)
{
    obl_object *result;
    obl_shape_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (obl_shape_storage*) malloc(sizeof(obl_shape_storage));
    if (storage == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        free(result);
        return NULL;
    }
    result->shape = NULL;

    storage->name = name;
    storage->slot_names = slot_names;
    storage->current_shape = obl_nil(d);
    storage->storage_format = (obl_uword) type;
    result->storage.shape_storage = storage;

    return result;
}

obl_object *obl_create_cshape(obl_database *d,
        char *name, size_t slot_count, char **slot_names,
        obl_storage_type type)
{
    obl_object *name_ob, *slots_ob, *slot_name_ob;
    obl_object *result;
    int i, j;

    name_ob = obl_create_cstring(d, name, strlen(name));
    if (name_ob == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a shape name.");
        return NULL;
    }
    slots_ob = obl_create_fixed(d, slot_count);
    if (slots_ob == NULL) {
        obl_destroy_object(name_ob);
        obl_report_error(d, OUT_OF_MEMORY,
                "Unable to allocate a shape's slot names.");
        return NULL;
    }

    for (i = 0; i < slot_count; i++) {
        slot_name_ob = obl_create_cstring(d, slot_names[i], strlen(slot_names[i]));
        if (slot_name_ob == NULL) {
            for (j = 0; j < i; j++) {
                obl_destroy_object(obl_fixed_at(slots_ob, j));
            }
            obl_destroy_object(slots_ob);
            obl_destroy_object(name_ob);
            obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a slot name.");
            return NULL;
        }
        obl_fixed_at_put(slots_ob, i, slot_name_ob);
    }

    result = obl_create_shape(d, name_ob, slots_ob, type);
    if (result == NULL) {
        for (j = 0; j < slot_count; j++) {
            obl_destroy_object(obl_fixed_at(slots_ob, j));
        }
        obl_destroy_object(slots_ob);
        obl_destroy_object(name_ob);
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate the shape itself.");
        return NULL;
    }

    return result;
}

/*
 * ============================================================================
 * Common operations on shapes and built-in collection types.
 * ============================================================================
 */

obl_uword obl_fixed_size(const obl_object *fixed)
{
    /* if (_storage_of(o) != OBL_FIXED_STORAGE) {
     return 0;
     } */

    return fixed->storage.fixed_storage->length;
}

obl_object *obl_fixed_at(const obl_object *fixed, const obl_uword index)
{
    /* if (_storage_of(o) != OBL_FIXED_STORAGE) {
     return 0;
     } */

    return fixed->storage.fixed_storage->contents[index];
}

void obl_fixed_at_put(obl_object *fixed, const obl_uword index, obl_object *value)
{
    if (_storage_of(fixed) != OBL_FIXED) {
        return ;
    }

    fixed->storage.fixed_storage->contents[index] = value;
}

size_t obl_string_size(const obl_object *string)
{
    if (_storage_of(string) != OBL_STRING) {
        return 0;
    }

    return string->storage.string_storage->length;
}

obl_uword obl_string_chars(const obl_object *string, char *buffer, size_t buffer_size)
{
    obl_string_storage *storage;
    UConverter *converter;
    obl_uword converted_length;
    UErrorCode status = U_ZERO_ERROR;

    if (_storage_of(string) != OBL_STRING) {
        return 0;
    }

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_error(string->database, OUT_OF_MEMORY,
                "Unable to allocate Unicode converter.");
        return 0;
    }

    storage = string->storage.string_storage;

    converted_length = ucnv_fromUChars(converter, buffer, buffer_size,
            storage->contents, storage->length, &status);
    ucnv_close(converter);

    if (U_FAILURE(status)) {
        obl_report_error(string->database, CONVERSION_ERROR,
                "Unable to convert string from UTF-16.");
    }

    return converted_length;
}

int obl_string_cmp(const obl_object *string_a, const obl_object *string_b)
{
    obl_uword length;

    if (_storage_of(string_a) != OBL_STRING || _storage_of(string_b) != OBL_STRING) {
        return 1;
    }

    length = obl_string_size(string_a);
    if (obl_string_size(string_b) != length) {
        return 2;
    }

    return memcmp(
            string_a->storage.string_storage->contents,
            string_b->storage.string_storage->contents,
            length * sizeof(UChar));
}

int obl_string_ccmp(const obl_object *string, const char *match)
{
    obl_object *temp;
    int result;

    temp = obl_create_cstring(string->database, match, strlen(match));
    if (temp == NULL) {
        obl_report_error(string->database, OUT_OF_MEMORY,
                "Unable to allocate a string for comparison");
        return -1;
    }
    result = obl_string_cmp(string, temp);
    obl_destroy_object(temp);

    return result;
}

obl_object *obl_slotted_at(const obl_object *slotted, const obl_uword index)
{
    return slotted->storage.slotted_storage->slots[index];
}

obl_object *obl_slotted_atnamed(const obl_object *slotted, const obl_object *slotname)
{
    return obl_slotted_at(slotted, obl_shape_slotnamed(slotted->shape, slotname));
}

obl_object *obl_slotted_atcnamed(const obl_object *slotted, const char *slotname)
{
    return obl_slotted_at(slotted, obl_shape_slotcnamed(slotted->shape, slotname));
}

void obl_slotted_at_put(obl_object *slotted, const obl_uword index,
        obl_object *value)
{
    slotted->storage.slotted_storage->slots[index] = value;
}

void obl_slotted_atnamed_put(obl_object *slotted, const obl_object *slotname,
        obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotnamed(slotted->shape, slotname), value);
}

void obl_slotted_atcnamed_put(obl_object *slotted, const char *slotname,
        obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotcnamed(slotted->shape, slotname), value);
}

obl_uword obl_shape_slotcount(const obl_object *shape)
{
    if (_storage_of(shape) != OBL_SHAPE) {
        return 0;
    }

    return obl_fixed_size(shape->storage.shape_storage->slot_names);
}

obl_uword obl_shape_slotnamed(const obl_object *shape, const obl_object *name)
{
    obl_object *slots;
    obl_uword i;

    slots = shape->storage.shape_storage->slot_names;
    for (i = 0; i < obl_fixed_size(slots); i++) {
        if (obl_string_cmp(obl_fixed_at(slots, i), name) == 0) {
            return i;
        }
    }

    return -1;
}

obl_uword obl_shape_slotcnamed(const obl_object *shape, const char *name)
{
    obl_object *temporary;
    obl_uword result;

    temporary = obl_create_cstring(shape->database, name, strlen(name));
    if (temporary == NULL) {
        return -1;
    }
    result = obl_shape_slotnamed(shape, temporary);
    obl_destroy_object(temporary);

    return result;
}

obl_storage_type obl_shape_storagetype(const obl_object *shape)
{
    return (obl_storage_type)
            shape->storage.shape_storage->storage_format;
}

/*
 * ============================================================================
 * Functions to translate obl_object values into primitive C types.
 * ============================================================================
 */

int obl_integer_value(const obl_object *integer)
{
    if (_storage_of(integer) != OBL_INTEGER) {
        return 0;
    }

    return (int) (integer->storage.integer_storage->value);
}

size_t obl_string_value(const obl_object *string, UChar *buffer, size_t buffer_size)
{
    obl_string_storage *storage;
    size_t count , bytes;

    if (_storage_of(string) != OBL_STRING) {
        return 0;
    }

    storage = string->storage.string_storage;
    count = storage->length > buffer_size ? storage->length : buffer_size;
    bytes = count * sizeof(UChar);

    memcpy(buffer, string->storage.string_storage->contents, bytes);

    return count;
}

/*
 * ============================================================================
 * Orderly object destruction.
 * ============================================================================
 */

void obl_destroy_object(obl_object *o)
{
    if (o->storage.any_storage != NULL ) {
        free(o->storage.any_storage);
    }
    free(o);
}

/* Free exactly those structures created by obl_create_shape. */
void obl_destroy_cshape(obl_object *shape)
{
    obl_shape_storage *storage;
    int slot_count, i;

    storage = shape->storage.shape_storage;
    obl_destroy_object(storage->name);

    slot_count = obl_fixed_size(storage->slot_names);
    for (i = 0; i < slot_count; i++) {
        obl_destroy_object(obl_fixed_at(storage->slot_names, i));
    }
    obl_destroy_object(storage->slot_names);
}

/*
 * ============================================================================
 * Private methods that should only be called elsewhere within ObjectLite.
 * ============================================================================
 */

obl_object *_obl_create_nil(obl_database *d)
{
    obl_object *result, *shape;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    result->shape = obl_at_address(d, OBL_NIL_SHAPE_ADDR);
    result->storage.nil_storage = NULL;

    return result;
}

obl_object *_obl_create_bool(obl_database *d, int truth)
{
    obl_object *result, *shape;
    obl_boolean_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (obl_boolean_storage*) malloc(sizeof(obl_boolean_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OUT_OF_MEMORY,
                "Unable to allocate a boolean");
        return NULL;
    }

    storage->value = truth;

    result->shape = obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR);
    result->storage.boolean_storage = storage;

    return result;
}

obl_object *_obl_create_stub(obl_database *d, obl_logical_address address)
{
    return NULL;
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
        return (obl_storage_type) o->shape->storage.shape_storage->storage_format;
    }
}

/* Allocate and perform common initialization for an unpersisted obl_object. */
inline obl_object *_allocate_object(obl_database *d)
{
    obl_object *result = (obl_object *) malloc(sizeof(obl_object));

    if (result == NULL) {
        obl_report_error(d, OUT_OF_MEMORY, "Unable to allocate a new object.");
        return NULL;
    }

    result->database = d;
    result->logical_address = 0;
    result->physical_address = 0;
    return result;
}

/* Allocate and perform common initialization for STRING objects. */
static obl_object *_allocate_string(obl_database *d, UChar *uc, obl_uword length)
{
    obl_object *result;
    obl_string_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
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

    storage->length = length;
    storage->contents = uc;

    return result;
}
