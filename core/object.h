/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in ObjectLite.
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "unicode/utypes.h"

#include <stdint.h>

#include "platform.h"

/*
 * Forward declarations.
 */

struct _obl_object;
typedef struct _obl_object obl_object;

/* Used to specify a physical word address within the .obl file. */
typedef obl_address obl_physical_address;

/* 
 * Used to specify the logical address of a remote object that's stored
 * elsewhere in the database.
 */
typedef obl_address obl_logical_address;

#include "constants.h"
#include "database.h"

/*
 * Possible values for kinds of internal storage.  Each of these corresponds to
 * a function in obl_read_functions, as defined in io.c, and an obl_shape_xxx
 * struct defined below.
 */
typedef enum
{
    OBL_SHAPE,

    OBL_SLOTTED, OBL_FIXED, OBL_CHUNK, OBL_TREEPAGE,

    OBL_INTEGER, OBL_FLOAT, OBL_DOUBLE, OBL_CHAR, OBL_STRING, OBL_BOOLEAN,

    OBL_NIL, OBL_STUB,

    OBL_STORAGE_TYPE_MAX = OBL_STUB
} obl_storage_type;

/*
 * Shape
 *
 * "Class" object which specifies how to interpret any object whose header word
 * points to it.
 */
typedef struct
{

    /* The shape's base name, including a language-agnostic namespace prefix. */
    obl_object *name;

    /*
     * A fixed-size collection of slot names in the order that they will occur
     * within instances.  Objects that have no slot names (such as fixed or
     * chunked objects) will have nil here.
     */
    obl_object *slot_names;

    /*
     * If non-nil, specifies the migration destination for instances of this
     * shape.  Instances will be migrated to this shape on read and persisted in
     * their new shape on write.
     */
    obl_object *current_shape;

    /*
     * The internal storage format to be used for I/O of instances of this shape.
     */
    uint32_t storage_format;

} obl_shape_storage;

/*
 * Slotted
 *
 * Contains zero to many "slots" (more commonly known as instance variables).
 * Each slot contains a reference to another object.  The number of slots and
 * the names of each slot are specified by the object's shape.
 */
typedef struct
{

    /*
     * An array of object references, mapped to +slot_names+ by position.  The
     * array size is determined by the number of slot names defined in the
     * object's shape.
     */
    obl_object *slots;

} obl_slotted_storage;

/*
 * Fixed
 *
 * Fixed, immutable length collection containing position-indexed references to
 * other objects.
 */
typedef struct
{

    /* The size of the collection. */
    uint32_t length;

    /* Collection payload. */
    obl_object **contents;

} obl_fixed_storage;

/*
 * Chunk
 *
 * Single section of a variable-length, position-indexed collection.  Chunks act
 * as a singly-linked list of nodes that contain batches of consecutive
 * collection contents.
 */
typedef struct
{

    /* The next chunk in the list. */
    obl_object *next;

    /* The contents of this chunk. */
    obl_object *contents[CHUNK_SIZE];

} obl_chunk_storage;

/*
 * Tree Page
 *
 * Building block for indices and hashes, include the address map and shape storage.
 */
typedef struct
{

    /* Position of the page within the tree.  Leaves have a depth of 0. */
    uint32_t depth;

    /* Object pointers.  On leaves, these will be tree contents; on branches,
     * pointers to the next level.
     */
    obl_object *contents[CHUNK_SIZE];

} obl_treepage_storage;

/*
 * Integer
 *
 * A signed integer value within the range +/- 2^31 - 1.
 */
typedef int32_t obl_integer_storage;

/*
 * Float
 *
 * Fractional number, stored in 32 bits as sign bit, exponent and mantissa.
 * Follows the IEEE 754-1985 standard explicitly to ensure binary compatibility
 * across compilers and architectures.  Of course, this may or may not actually
 * correspond to a native C float for any particular build, so conversions
 * should be done with caution.
 *
 * The numeric value represented can be calculated as:
 *   (-1)^sign * 2^(exponent - 127) * (1 + mantissa)
 * If the number is normalized (exponent == 0 and mantissa != 0), or:
 *   (-1)^sign * 2^(-126) * (0 + mantissa)
 * if it is denormalized.  There are other special cases for signed 0s, NaN,
 * and the infinities which you can read about below.
 *
 * Reference: http://en.wikipedia.org/wiki/IEEE_754-1985#Single-precision_32-bit
 */
typedef struct
{

    /* Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /* Exponent, biased with 127. */
    unsigned int exponent :8;

    /* 1.mantissa in binary. */
    unsigned int mantissa :23;

} obl_float_storage;

/*
 * Double
 *
 * Double-precision floating point number, stored in 64 bits.  Storage is similar
 * to the single-precision Float with a longer exponent and mantissa.
 *
 * Reference: http://en.wikipedia.org/wiki/IEEE_754-1985#Double-precision_64-bit
 */
typedef struct
{

    /* Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /* Exponent, biased with +1023 (-1022 if denormalized). */
    unsigned int exponent :11;

    /* 1.mantissa in binary. */
    unsigned long long mantissa :52;

} obl_double_storage;

/*
 * Char
 *
 * Single unicode character (not a single code point).
 */
typedef UChar32 obl_char_storage;

/*
 * String
 *
 * Length-prefixed UTF-16 string.
 */
typedef struct
{

    /* Size of the string, in code points (not in characters). */
    uint32_t length;

    /* Array of code points. */
    UChar *contents;

} obl_string_storage;

/*
 * Boolean
 *
 * Truth or falsehood.  Boolean objects are never instantiated: rather, the
 * objects True and False reside at fixed logical addresses and are returned
 * from abstract I/O functions as needed.
 */
typedef uint32_t obl_boolean_storage;

/*
 * Nil
 *
 * The null object.  Like Booleans, nil is never instantiated directly, but has
 * one instance accessable at the logical address 0.  Actually, nil has no
 * internal storage at all: it's represented by a null pointer in the storage
 * field.
 */
typedef uint32_t obl_nil_storage;

/*
 * Stub
 *
 * Stand in for an object that has not yet been loaded.  The slots of objects
 * that are too deep in the object graph to load directly are instead populated
 * by psuedo-objects with stub storage.  Stubs contain only the logical address
 * of the real object.
 *
 * Client code should never see an obl_object with stub storage, because the
 * object access API resolves them as they are seen.
 */
typedef obl_logical_address obl_stub_storage;

/*
 * A wrapper that contains an object's shape and internal storage.  Most
 * external and language binding code should work with obl_object objects
 * and use the functions provided here to manipulate them.
 */
struct _obl_object
{

    /* Database that this object is stored in, or NULL. */
    obl_database *database;

    /* The logical address of this object, if one has been assigned. */
    obl_logical_address logical_address;

    /* The physical address within the database, if this instance is persisted. */
    obl_physical_address physical_address;

    /* Shape of this instance. */
    obl_object *shape;

    /* Internal data storage.  The active internal storage module is determined by
     * the +shape+ of the instance (NULL indicates shape storage).
     */
    union
    {
        obl_shape_storage *shape_storage;

        obl_slotted_storage *slotted_storage;
        obl_fixed_storage *fixed_storage;
        obl_chunk_storage *chunk_storage;
        obl_treepage_storage *treepage_storage;

        obl_integer_storage *integer_storage;
        obl_float_storage *float_storage;
        obl_double_storage *double_storage;

        obl_char_storage *char_storage;
        obl_string_storage *string_storage;

        obl_boolean_storage *boolean_storage;
        obl_nil_storage *nil_storage;
        obl_stub_storage *stub_storage;
    } storage;
};

/*
 * ============================================================================
 * Translate primitive C types into their equivalent obl_object structures.
 * ============================================================================
 */

/* int to INTEGER object. */
obl_object *obl_create_integer(obl_database *d, int i);

/* float to FLOAT object. */
obl_object *obl_create_float(obl_database *d, float f);

/* double to DOUBLE object. */
obl_object *obl_create_double(obl_database *d, double dbl);

/* char to CHAR object, including translation from US-ASCII to UTF-16. */
obl_object *obl_create_char(obl_database *d, char c);

/* Unicode UChar32 to CHAR object. */
obl_object *obl_create_uchar(obl_database *d, UChar32 uc);

/* NULL-terminated C string to UTF-16 STRING object. */
obl_object *obl_create_string(obl_database *d, const char *c, int32_t length);

/* NULL-terminated unicode string to STRING object. */
obl_object *obl_create_ustring(obl_database *d, const UChar *uc, int32_t length);

/* Fixed-size collection creation. */
obl_object *obl_create_fixed(obl_database *d, uint32_t length);

/* Placeholder for deferring an object load operation. */
obl_object *obl_create_stub(obl_database *d, obl_logical_address address);

/* Direct creation of SHAPE objects, for convenience. */
obl_object *obl_create_shape(obl_database *d, char *name, char **slot_names,
        obl_storage_type type);

/*
 * ============================================================================
 * Common operations on shapes and built-in collection types.
 * ============================================================================
 */

uint32_t obl_fixed_size(obl_object *o);

obl_object *obl_fixed_at(obl_object *o, const uint32_t index);

void obl_fixed_at_put(obl_object *o, const uint32_t index, obl_object *value);

/*
 * ============================================================================
 * Translate obl_object structures into primitive C types.
 * ============================================================================
 */

int obl_integer_value(const obl_object *o);

/* Return the number of code points contained in the STRING object +o+. */
size_t obl_string_size(const obl_object *o);

/* Acquire at most +buffer_size+ code points contained within a STRING object
 * +o+ into a prepared +buffer+.  Return the number of code points copied.
 */
size_t obl_string_value(const obl_object *o, UChar *buffer, size_t buffer_size);

/* Acquire at most +buffer_size+ US-ASCII characters into +buffer+.  Return the
 * number of code points copied.
 */
size_t obl_string_chars(const obl_object *o, char *buffer, size_t buffer_size);

/* Orderly obl_object deallocation, including nested structures (but not linked
 * objects, such as the shape or slot contents).
 */
void obl_destroy_object(obl_object *o);

#endif
