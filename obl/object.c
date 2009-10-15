/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in
 * ObjectLite.
 */

#include "object.h"

#include "database.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

#include "unicode/ucnv.h"

/*
 * ============================================================================
 * Prototypes for internal functions.
 * ============================================================================
 */

static inline struct obl_object *_allocate_object(struct obl_database *d);

static struct obl_object *_allocate_string(struct obl_database *d,
        UChar *uc, obl_uint length);

/*
 * ============================================================================
 * Object creation functions.
 * ============================================================================
 */

struct obl_object *obl_create_integer(struct obl_database *d, obl_int i)
{
    struct obl_object *result;
    struct obl_integer_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_integer_storage *)
            malloc(sizeof(struct obl_integer_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }

    result->shape = obl_at_address(d, OBL_INTEGER_SHAPE_ADDR);
    storage->value = i;
    result->storage.integer_storage = storage;
    return result;
}

struct obl_object *obl_create_float(struct obl_database *d, float f)
{
    return NULL;
}

struct obl_object *obl_create_double(struct obl_database *d, double dbl)
{
    return NULL;
}

struct obl_object *obl_create_char(struct obl_database *d, char c)
{
    return NULL;
}

struct obl_object *obl_create_uchar(struct obl_database *d, UChar32 uc)
{
    return NULL;
}

struct obl_object *obl_create_string(struct obl_database *d,
        const UChar *uc, obl_uint length)
{
    UChar *copied;
    size_t bytes;

    bytes = sizeof(UChar) * (size_t) length;
    copied = (UChar *) malloc(bytes);
    if( copied == NULL ) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    memcpy(copied, uc, bytes);

    return _allocate_string(d, copied, length);
}

struct obl_object *obl_create_cstring(struct obl_database *d,
        const char *c, obl_uint length)
{
    UConverter *converter;
    size_t output_length;
    UChar *output_string;
    size_t converted_length;
    UErrorCode status = U_ZERO_ERROR;

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_errorf(d, OBL_CONVERSION_ERROR,
                "Error creating Unicode converter: %s",
                u_errorName(status));
        return NULL;
    }

    output_length = (size_t) length * 2;
    output_string = (UChar *) malloc(sizeof(UChar) * output_length);
    if (output_string == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    converted_length = ucnv_toUChars(converter, output_string, output_length,
            c, length, &status);
    ucnv_close(converter);
    if (U_FAILURE(status)) {
        obl_report_errorf(d, OBL_CONVERSION_ERROR,
                "Unicode conversion failure: %s",
                u_errorName(status));
        free(output_string);
        return NULL;
    }

    return _allocate_string(d, output_string, (obl_uint) converted_length);
}

struct obl_object *obl_create_fixed(struct obl_database *d, obl_uint length)
{
    struct obl_object *result;
    struct obl_fixed_storage *storage;
    obl_uint i;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_fixed_storage *)
            malloc(sizeof(struct obl_fixed_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.fixed_storage = storage;
    result->shape = obl_at_address(d, OBL_FIXED_SHAPE_ADDR);

    storage->length = length;
    storage->contents = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * length);
    if (storage->contents == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        free(storage);
        return NULL;
    }

    for (i = 0; i < length; i++) {
        storage->contents[i] = obl_nil(d);
    }

    return result;
}

struct obl_object *obl_create_addrtreepage(struct obl_database *d,
        obl_uint depth)
{
    struct obl_object *result;
    struct obl_addrtreepage_storage *storage;
    obl_uint i;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }
    result->shape = obl_at_address(d, OBL_ADDRTREEPAGE_SHAPE_ADDR);

    storage = (struct obl_addrtreepage_storage*)
            malloc(sizeof(struct obl_addrtreepage_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    storage->height = depth;
    for (i = 0; i < CHUNK_SIZE; i++) {
        storage->contents[i] = OBL_PHYSICAL_UNASSIGNED;
    }
    result->storage.addrtreepage_storage = storage;

    return result;
}

struct obl_object *obl_create_slotted(struct obl_object *shape)
{
    struct obl_object *result;
    struct obl_slotted_storage *storage;
    obl_uint slot_count;
    struct obl_object **slots;
    obl_uint i;

    if (_obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_create_slotted requires a SHAPE object.");
        return NULL;
    }

    result = _allocate_object(shape->database);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_slotted_storage*)
            malloc(sizeof(struct obl_slotted_storage));
    if (storage == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.slotted_storage = storage;
    result->shape = shape;

    slot_count = obl_shape_slotcount(shape);
    slots = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * slot_count);
    if (slots == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
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

struct obl_object *obl_create_shape(struct obl_database *d,
        struct obl_object *name, struct obl_object *slot_names,
        obl_storage_type type)
{
    struct obl_object *result;
    struct obl_shape_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_shape_storage*)
            malloc(sizeof(struct obl_shape_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->shape = obl_nil(d);

    storage->name = name;
    storage->slot_names = slot_names;
    storage->current_shape = obl_nil(d);
    storage->storage_format = (obl_uint) type;
    result->storage.shape_storage = storage;

    return result;
}

struct obl_object *obl_create_cshape(struct obl_database *d,
        char *name, size_t slot_count, char **slot_names,
        obl_storage_type type)
{
    struct obl_object *name_ob, *slots_ob, *slot_name_ob;
    struct obl_object *result;
    int i, j;

    name_ob = obl_create_cstring(d, name, strlen(name));
    if (name_ob == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    slots_ob = obl_create_fixed(d, slot_count);
    if (slots_ob == NULL) {
        obl_destroy_object(name_ob);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    for (i = 0; i < slot_count; i++) {
        slot_name_ob = obl_create_cstring(d,
                slot_names[i], strlen(slot_names[i]));
        if (slot_name_ob == NULL) {
            for (j = 0; j < i; j++) {
                obl_destroy_object(obl_fixed_at(slots_ob, j));
            }
            obl_destroy_object(slots_ob);
            obl_destroy_object(name_ob);
            obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
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
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    return result;
}

/*
 * ============================================================================
 * Common operations on shapes and built-in collection types.
 * ============================================================================
 */

obl_uint obl_fixed_size(const struct obl_object *fixed)
{
    if (_obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                "obl_fixed_size requires an object with FIXED storage.");
        return 0;
    }

    return fixed->storage.fixed_storage->length;
}

struct obl_object *obl_fixed_at(const struct obl_object *fixed,
        const obl_uint index)
{
    if (_obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return NULL;
    }

    return fixed->storage.fixed_storage->contents[index];
}

void obl_fixed_at_put(struct obl_object *fixed, const obl_uint index,
        struct obl_object *value)
{
    if (_obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(fixed->database, OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return ;
    }

    fixed->storage.fixed_storage->contents[index] = value;
}

obl_uint obl_string_size(const struct obl_object *string)
{
    if (_obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_size requires an object with STRING storage.");
        return 0;
    }

    return string->storage.string_storage->length;
}

size_t obl_string_chars(const struct obl_object *string,
        char *buffer, size_t buffer_size)
{
    struct obl_string_storage *storage;
    UConverter *converter;
    obl_uint converted_length;
    UErrorCode status = U_ZERO_ERROR;

    if (_obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_chars requires an object with STRING storage.");
        return 0;
    }

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_errorf(string->database, OBL_CONVERSION_ERROR,
                "Error opening Unicode converter: %s",
                u_errorName(status));
        return 0;
    }

    storage = string->storage.string_storage;

    converted_length = ucnv_fromUChars(converter, buffer, buffer_size,
            storage->contents, storage->length, &status);
    ucnv_close(converter);

    if (U_FAILURE(status)) {
        obl_report_errorf(string->database, OBL_CONVERSION_ERROR,
                "Unable to convert string from UTF-16: %s",
                u_errorName(status));
    }

    return converted_length;
}

int obl_string_cmp(const struct obl_object *string_a,
        const struct obl_object *string_b)
{
    obl_uint length;

    if (_obl_storage_of(string_a) != OBL_STRING || _obl_storage_of(string_b) != OBL_STRING) {
        return -1;
    }

    length = obl_string_size(string_a);
    if (obl_string_size(string_b) != length) {
        return -1;
    }

    return memcmp(
            string_a->storage.string_storage->contents,
            string_b->storage.string_storage->contents,
            length * sizeof(UChar));
}

int obl_string_ccmp(const struct obl_object *string, const char *match)
{
    struct obl_object *temp;
    int result;

    if (_obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_ccmp requires a STRING object.");
        return -1;
    }

    temp = obl_create_cstring(string->database, match, strlen(match));
    if (temp == NULL) {
        obl_report_error(string->database, OBL_OUT_OF_MEMORY, NULL);
        return -1;
    }
    result = obl_string_cmp(string, temp);
    obl_destroy_object(temp);

    return result;
}

struct obl_object *obl_slotted_at(const struct obl_object *slotted,
        const obl_uint index)
{
    if (_obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(slotted->database, OBL_WRONG_STORAGE,
                "obl_slotted_at requires a SLOTTED object.");
        return obl_nil(slotted->database);
    }

    return slotted->storage.slotted_storage->slots[index];
}

struct obl_object *obl_slotted_atnamed(const struct obl_object *slotted,
        const struct obl_object *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotnamed(slotted->shape, slotname));
}

struct obl_object *obl_slotted_atcnamed(const struct obl_object *slotted,
        const char *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotcnamed(slotted->shape, slotname));
}

void obl_slotted_at_put(struct obl_object *slotted,
        const obl_uint index, struct obl_object *value)
{
    if (_obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(slotted->database, OBL_WRONG_STORAGE,
                "obl_slotted_at_put requires a SLOTTED object.");
        return ;
    }

    slotted->storage.slotted_storage->slots[index] = value;
}

void obl_slotted_atnamed_put(struct obl_object *slotted,
        const struct obl_object *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotnamed(slotted->shape, slotname), value);
}

void obl_slotted_atcnamed_put(struct obl_object *slotted,
        const char *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotcnamed(slotted->shape, slotname), value);
}

obl_uint obl_shape_slotcount(const struct obl_object *shape)
{
    if (_obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcount invoked with a non SHAPE object.");
        return 0;
    }

    return obl_fixed_size(shape->storage.shape_storage->slot_names);
}

obl_uint obl_shape_slotnamed(const struct obl_object *shape,
        const struct obl_object *name)
{
    struct obl_object *slots;
    obl_uint i;

    if (_obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotnamed invoked with a non SHAPE object.");
        return 0;
    }

    slots = shape->storage.shape_storage->slot_names;
    for (i = 0; i < obl_fixed_size(slots); i++) {
        if (obl_string_cmp(obl_fixed_at(slots, i), name) == 0) {
            return i;
        }
    }

    return -1;
}

obl_uint obl_shape_slotcnamed(const struct obl_object *shape,
        const char *name)
{
    struct obl_object *temporary;
    obl_uint result;

    if (_obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcnamed invoked with a non SHAPE object.");
        return 0;
    }

    temporary = obl_create_cstring(shape->database, name, strlen(name));
    if (temporary == NULL) {
        return -1;
    }
    result = obl_shape_slotnamed(shape, temporary);
    obl_destroy_object(temporary);

    return result;
}

obl_storage_type obl_shape_storagetype(const struct obl_object *shape)
{
    if (_obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_storagetype invoked with a non SHAPE object.");
        return 0;
    }

    return (obl_storage_type)
            shape->storage.shape_storage->storage_format;
}

/*
 * ============================================================================
 * Functions to translate struct obl_object values into primitive C types.
 * ============================================================================
 */

obl_int obl_integer_value(const struct obl_object *integer)
{
    if (_obl_storage_of(integer) != OBL_INTEGER) {
        obl_report_error(integer->database, OBL_WRONG_STORAGE,
                "obl_integer_value called with a non-INTEGER object.");
        return 0;
    }

    return integer->storage.integer_storage->value;
}

int obl_boolean_value(const struct obl_object *bool)
{
    if (_obl_storage_of(bool) != OBL_BOOLEAN) {
        OBL_WARN(bool->database, "Non-boolean object: assuming truth.");
        return 1;
    }

    return (int) (bool->storage.boolean_storage->value);
}

size_t obl_string_value(const struct obl_object *string,
        UChar *buffer, size_t buffer_size)
{
    struct obl_string_storage *storage;
    size_t count , bytes;

    if (_obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_value called with a non-STRING object.");
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
 * Various methods of object destruction.
 * ============================================================================
 */

void obl_destroy_object(struct obl_object *o)
{
    if (o->storage.any_storage != NULL ) {
        free(o->storage.any_storage);
    }
    free(o);
}

/* Free exactly those structures created by obl_create_shape. */
void obl_destroy_cshape(struct obl_object *shape)
{
    struct obl_shape_storage *storage;
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

struct obl_object *_obl_create_nil(struct obl_database *d)
{
    struct obl_object *result, *shape;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    result->shape = obl_at_address(d, OBL_NIL_SHAPE_ADDR);
    result->storage.nil_storage = NULL;

    return result;
}

struct obl_object *_obl_create_bool(struct obl_database *d, int truth)
{
    struct obl_object *result, *shape;
    struct obl_boolean_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_boolean_storage*)
            malloc(sizeof(struct obl_boolean_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    storage->value = truth;

    result->shape = obl_at_address(d, OBL_BOOLEAN_SHAPE_ADDR);
    result->storage.boolean_storage = storage;

    return result;
}

struct obl_object *_obl_create_stub(struct obl_database *d,
        obl_logical_address address)
{
    struct obl_object *result;
    struct obl_stub_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_stub_storage*)
            malloc(sizeof(struct obl_stub_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    storage->value = address;

    result->shape = obl_at_address(d, OBL_STUB_SHAPE_ADDR);
    result->storage.stub_storage = storage;
    result->logical_address = address;

    return result;
}

obl_storage_type _obl_storage_of(const struct obl_object *o)
{
    if (o->shape == obl_nil(o->database)) {
        return OBL_SHAPE;
    } else {
        return (obl_storage_type)
                o->shape->storage.shape_storage->storage_format;
    }
}

struct obl_object *_obl_resolve_stub(struct obl_object *stub, int depth)
{
    return obl_at_address_depth(stub->database,
            stub->storage.stub_storage->value,
            depth);
}

int _obl_is_stub(struct obl_object *o)
{
    struct obl_object *shape;

    shape = o->shape;
    if (shape == obl_nil(o->database)) {
        return 0;
    } else {
        return obl_shape_storagetype(shape) == OBL_STUB;
    }
}

/*
 * ============================================================================
 * Internal function definitions.
 * ============================================================================
 */

/*
 * Allocate and perform common initialization for an unpersisted obl_object.
 */
inline struct obl_object *_allocate_object(struct obl_database *d)
{
    struct obl_object *result = (struct obl_object *)
            malloc(sizeof(struct obl_object));

    if (result == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    result->database = d;
    result->logical_address = OBL_LOGICAL_UNASSIGNED;
    result->physical_address = OBL_PHYSICAL_UNASSIGNED;
    return result;
}

/* Allocate and perform common initialization for STRING objects. */
static struct obl_object *_allocate_string(struct obl_database *d,
        UChar *uc, obl_uint length)
{
    struct obl_object *result;
    struct obl_string_storage *storage;

    result = _allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_string_storage *)
            malloc(sizeof(struct obl_string_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.string_storage = storage;
    result->shape = obl_at_address(d, OBL_STRING_SHAPE_ADDR);

    storage->length = length;
    storage->contents = uc;

    return result;
}
