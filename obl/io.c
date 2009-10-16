/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#include "io.h"

#include "database.h"
#include "object.h"
#include "platform.h"

#include <stdlib.h>

/*
 * The array of object-reading functions: one for each internal state
 * specified in "object.h" and at the same index as its index in the
 * +obl_storage_type+ enumeration.
 */
static obl_object_read_function obl_read_functions[] = {
        &obl_read_shape,        /* OBL_SHAPE */
        &obl_read_slotted,      /* OBL_SLOTTED */
        &obl_invalid_read,      /* OBL_FIXED */
        &obl_invalid_read,      /* OBL_CHUNK */
        &obl_read_addrtreepage, /* OBL_ADDRTREEPAGE */
        &obl_read_integer,      /* OBL_INTEGER */
        &obl_invalid_read,      /* OBL_FLOAT */
        &obl_invalid_read,      /* OBL_DOUBLE */
        &obl_invalid_read,      /* OBL_CHAR */
        &obl_read_string,       /* OBL_STRING */
        &obl_invalid_read,      /* OBL_BOOLEAN (invalid) */
        &obl_invalid_read,      /* OBL_NIL (invalid) */
        &obl_invalid_read       /* OBL_STUB (invalid) */
};

/*
 * The array of object-writing functions.  Each function serializes an object
 * of a certain storage type, not including its header byte word, to the
 * location specified by its pre-set physical address.
 */
static obl_object_write_function obl_write_functions[] = {
        &obl_write_shape,        /* OBL_SHAPE */
        &obl_write_slotted,      /* OBL_SLOTTED */
        &obl_write_fixed,        /* OBL_FIXED */
        &obl_invalid_write,      /* OBL_CHUNK */
        &obl_write_addrtreepage, /* OBL_ADDRTREEPAGE */
        &obl_write_integer,      /* OBL_INTEGER */
        &obl_invalid_write,      /* OBL_FLOAT */
        &obl_invalid_write,      /* OBL_DOUBLE */
        &obl_invalid_write,      /* OBL_CHAR */
        &obl_write_string,       /* OBL_STRING */
        &obl_invalid_write,      /* OBL_BOOLEAN (invalid) */
        &obl_invalid_write,      /* OBL_NIL (invalid) */
        &obl_invalid_write       /* OBL_STUB (invalid) */
};

/* Integers are stored in 32 bits, network byte order. */
struct obl_object *obl_read_integer(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    obl_int value;

    value = readable_int(source[offset]);

    return obl_create_integer(shape->database, value);
}

/* Strings are stored as UTF-16BE with a one-word length prefix. */
struct obl_object *obl_read_string(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    obl_uint length;
    obl_uint i;
    UChar *casted_source;
    int casted_offset;
    UChar *contents;
    struct obl_object *o;

    length = readable_uint(source[offset]);
    contents = (UChar*) malloc(length * sizeof(UChar));
    if (contents == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
        return obl_nil(shape->database);
    }

    casted_source = (UChar *) source;
    casted_offset = (offset + 1) * (sizeof(obl_uint) / sizeof(UChar));
    for (i = 0; i < length; i++) {
        contents[i] = readable_UChar(
                casted_source[casted_offset + i]);
    }

    o = obl_create_string(shape->database, contents, length);

    free(contents);
    return o;
}

struct obl_object *obl_read_slotted(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    struct obl_object *result;
    obl_uint slot_count;
    obl_uint i;
    obl_logical_address addr;
    struct obl_object *linked;

    result = obl_create_slotted(shape);

    slot_count = obl_shape_slotcount(shape);
    for (i = 0; i < slot_count; i++) {
        addr = (obl_logical_address) readable_uint(source[offset + i]);
        if (depth <= 0) {
            linked = _obl_create_stub(shape->database, addr);
        } else {
            linked = obl_at_address_depth(shape->database, addr, depth - 1);
        }
        obl_slotted_at_put(result, i, linked);
    }

    return result;
}

struct obl_object *obl_read_fixed(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    obl_uint length;
    obl_uint i;
    struct obl_object *o;
    obl_logical_address addr;
    struct obl_object *linked;

    length = readable_uint(source[offset]);
    o = obl_create_fixed(shape->database, length);

    for (i = 0; i < length; i++) {
        addr = (obl_logical_address) readable_uint(source[offset + 1 + i]);
        if (depth <= 0) {
            linked = _obl_create_stub(shape->database, addr);
        } else {
            linked = obl_at_address_depth(shape->database, addr, depth - 1);
        }
        obl_fixed_at_put(o, i, linked);
    }

    return o;
}

struct obl_object *obl_read_shape(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    struct obl_object *result;
    obl_logical_address addr;
    struct obl_object *name, *slot_names, *current_shape;
    obl_uint storage_format;

    addr = (obl_logical_address) readable_uint(source[offset]);
    name = obl_at_address_depth(shape->database, addr, depth - 1);

    addr = (obl_logical_address) readable_uint(source[offset + 1]);
    slot_names = obl_at_address_depth(shape->database, addr, depth - 1);

    addr = (obl_logical_address) readable_uint(source[offset + 2]);
    current_shape = obl_at_address_depth(shape->database, addr, depth - 1);

    storage_format = readable_uint(source[offset + 3]);
    if (storage_format > OBL_STORAGE_TYPE_MAX) {
        obl_report_errorf(shape->database, OBL_WRONG_STORAGE,
                "Shape at physical address %ul has invalid storage format.",
                (unsigned long) offset);
    }

    result = obl_create_shape(shape->database,
            name, slot_names, storage_format);
    result->storage.shape_storage->current_shape = current_shape;
    return result;
}

struct obl_object *obl_read_addrtreepage(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    struct obl_object *result;
    obl_uint height;
    obl_physical_address addr;
    int i;

    height = _obl_read_addrtreepage_height(source, offset);
    result = obl_create_addrtreepage(shape->database, height);

    for (i = 0; i < CHUNK_SIZE; i++) {
        addr = _obl_read_addrtreepage_at(source, offset, i);
        result->storage.addrtreepage_storage->contents[i] = addr;
    }

    return result;
}

struct obl_object *obl_invalid_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth)
{
    obl_report_errorf(shape->database, OBL_WRONG_STORAGE,
            "Attempt to read an object (%lu) with an invalid storage type.",
            offset);
    return obl_nil(shape->database);
}

struct obl_object *obl_read_object(struct obl_database *d,
        obl_uint *source, obl_physical_address offset, int depth)
{
    struct obl_object *shape, *result;
    obl_logical_address addr;
    int function_index;

    addr = (obl_logical_address) readable_uint(source[offset]);
    shape = obl_at_address_depth(d, addr, depth - 1);

    if (shape != obl_nil(d) && _obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_errorf(d, OBL_WRONG_STORAGE,
                "Corrupt shape header at physical address %ul.",
                offset);
        return obl_nil(d);
    }

    if (shape == obl_nil(d)) {
        function_index = OBL_SHAPE;
    } else {
        function_index = obl_shape_storagetype(shape);
    }

    result = (obl_read_functions[function_index])(
            shape, source, offset + 1, depth);
    result->shape = shape;
    result->physical_address = offset;

    return result;
}

void obl_write_integer(struct obl_object *integer, obl_uint *dest)
{
    obl_int value;

    value = obl_integer_value(integer);
    dest[integer->physical_address + 1] = writable_int(value);
}

void obl_write_string(struct obl_object *string, obl_uint *dest)
{
    obl_uint length;
    obl_uint i;
    UChar *casted_dest;
    obl_physical_address casted_offset;
    UChar ch;

    length = obl_string_size(string);
    dest[string->physical_address + 1] = writable_uint(length);

    casted_dest = (UChar *) dest;
    casted_offset = (string->physical_address + 2) *
            (sizeof(obl_uint) / sizeof(UChar));

    for (i = 0; i < length; i++) {
        ch = string->storage.string_storage->contents[i];
        casted_dest[casted_offset + i] = writable_UChar(ch);
    }
}

void obl_write_slotted(struct obl_object *slotted, obl_uint *dest)
{
    obl_uint slot_count;
    obl_uint i;
    struct obl_object *linked;

    slot_count = obl_shape_slotcount(slotted->shape);
    for (i = 0; i < slot_count; i++) {
        /* Avoid unnecessary stub resolution. */
        linked = slotted->storage.slotted_storage->slots[i];

        /*
         * TODO: Recursively persist +linked+ if it isn't already persisted.
         * Depends on the implementation of a generate +obl_write+ function
         * in database.h, which takes care of address assignment and so on.
         */
        dest[slotted->physical_address + 1 + i] = writable_uint(
                (obl_uint) linked->logical_address);
    }
}

void obl_write_fixed(struct obl_object *fixed, obl_uint *dest)
{
    obl_uint length;
    obl_uint i;
    struct obl_object *linked;

    length = obl_fixed_size(fixed);
    dest[fixed->physical_address + 1] = writable_uint(length);

    for (i = 0; i < length; i++) {
        /* Avoid unnecessarily resolving any stubs. */
        linked = fixed->storage.fixed_storage->contents[i];

        /*
         * TODO: Recursively persist +linked+ if it is not already persisted.
         * For now, assume it is.
         */
        dest[fixed->physical_address + 2 + i] = writable_uint(
                (obl_uint) linked->logical_address);
    }
}

void obl_write_shape(struct obl_object *shape, obl_uint *dest)
{
    struct obl_object *name, *slot_names, *current_shape;

    name = shape->storage.shape_storage->name;
    slot_names = shape->storage.shape_storage->slot_names;
    current_shape = shape->storage.shape_storage->current_shape;

    dest[shape->physical_address + 1] = writable_uint(
            (obl_uint) name->logical_address);
    dest[shape->physical_address + 2] = writable_uint(
            (obl_uint) slot_names->logical_address);
    dest[shape->physical_address + 3] = writable_uint(
            (obl_uint) current_shape->logical_address);

    dest[shape->physical_address + 4] = writable_uint(
            (obl_uint) obl_shape_storagetype(shape));
}

void obl_write_addrtreepage(struct obl_object *treepage, obl_uint *dest)
{
    int i;
    obl_physical_address base;
    obl_physical_address *contents;

    base = treepage->physical_address;
    dest[base + 1] =
            writable_uint(treepage->storage.addrtreepage_storage->height);

    contents = treepage->storage.addrtreepage_storage->contents;
    for (i = 0; i < CHUNK_SIZE; i++) {
        _obl_write_addrtreepage_at(dest, base, i, contents[i]);
    }
}

/*
 * Invoked for any storage type that is either not defined yet, or isn't
 * supposed to actually be written to the database.
 */
void obl_invalid_write(struct obl_object *o, obl_uint *dest)
{
    obl_report_errorf(o->database, OBL_WRONG_STORAGE,
            "Attempt to write an object with an invalid storage type (%lu).",
            _obl_storage_of(o));
}

/*
 * Writes the shape address and delegates to the appropriate write function
 * for this object's storage type.
 */
void obl_write_object(struct obl_object *o, obl_uint *dest)
{
    struct obl_object *shape;
    int function_index;

    if (_obl_is_stub(o->shape)) {
        shape = _obl_resolve_stub(o->shape, o->database->default_stub_depth);
        obl_destroy_object(o->shape);
        o->shape = shape;
    } else {
        shape = o->shape;
    }

    if (shape != obl_nil(o->database) && _obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(o->database, OBL_WRONG_STORAGE,
                "Attempt to write an object with a shape that isn't a shape.");
        return ;
    }

    if (shape != obl_nil(o->database)) {
        function_index = (int) obl_shape_storagetype(shape);
    } else {
        function_index = (int) OBL_SHAPE;
    }

    dest[o->physical_address] = writable_uint(
            (obl_uint) shape->logical_address);

    (*obl_write_functions[function_index])(o, dest);
}

obl_uint _obl_read_addrtreepage_height(obl_uint *source,
        obl_physical_address base)
{
    return readable_uint(source[base]);
}

obl_physical_address _obl_read_addrtreepage_at(obl_uint *source,
        obl_physical_address base, obl_uint index)
{
    return (obl_physical_address) readable_uint(source[base + 1 + index]);
}

void _obl_write_addrtreepage_at(obl_uint *dest,
        obl_physical_address base, obl_uint index, obl_physical_address value)
{
    dest[base + 2 + index] = writable_uint((obl_uint) value);
}
