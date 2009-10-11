/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in
 * ObjectLite.
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#include "constants.h"
#include "platform.h"

#include "unicode/utypes.h"

/* Defined in database.h */
struct obl_database;

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
struct obl_shape_storage {

    /* The shape's base name, including a language-agnostic namespace prefix. */
    struct obl_object *name;

    /*
     * A fixed-size collection of slot names in the order that they will occur
     * within instances.  Objects that have no slot names (such as fixed or
     * chunked objects) will have nil here.
     */
    struct obl_object *slot_names;

    /*
     * If non-nil, specifies the migration destination for instances of this
     * shape.  Instances will be migrated to this shape on read and persisted in
     * their new shape on write.
     */
    struct obl_object *current_shape;

    /*
     * The internal storage format to be used for I/O of instances of this shape.
     */
    obl_uint storage_format;

};

/*
 * Slotted
 *
 * Contains zero to many "slots" (more commonly known as instance variables).
 * Each slot contains a reference to another object.  The number of slots and
 * the names of each slot are specified by the object's shape.
 */
struct obl_slotted_storage {

    /*
     * An array of object references, mapped to +slot_names+ by position.  The
     * array size is determined by the number of slot names defined in the
     * object's shape.
     */
    struct obl_object **slots;

};

/*
 * Fixed
 *
 * Fixed, immutable length collection containing position-indexed references to
 * other objects.
 */
struct obl_fixed_storage {

    /* The size of the collection. */
    obl_uint length;

    /* Collection payload. */
    struct obl_object **contents;

};

/*
 * Chunk
 *
 * Single section of a variable-length, position-indexed collection.  Chunks act
 * as a singly-linked list of nodes that contain batches of consecutive
 * collection contents.
 */
struct obl_chunk_storage {

    /* The next chunk in the list. */
    struct obl_object *next;

    /* The contents of this chunk. */
    struct obl_object *contents[CHUNK_SIZE];

};

/*
 * Tree Page
 *
 * Building block for indices and hashes, include the address map and shape storage.
 */
struct obl_treepage_storage {

    /* Position of the page within the tree.  Leaves have a depth of 0. */
    obl_uint depth;

    /* Object pointers.  On leaves, these will be tree contents; on branches,
     * pointers to the next level.
     */
    struct obl_object *contents[CHUNK_SIZE];

};

/*
 * Integer
 *
 * A signed integer value within the range +/- 2^31 - 1.
 */
struct obl_integer_storage {
    obl_int value;
};

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
struct obl_float_storage {

    /* Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /* Exponent, biased with 127. */
    unsigned int exponent :8;

    /* 1.mantissa in binary. */
    unsigned int mantissa :23;

};

/*
 * Double
 *
 * Double-precision floating point number, stored in 64 bits.  Storage is similar
 * to the single-precision Float with a longer exponent and mantissa.
 *
 * Reference: http://en.wikipedia.org/wiki/IEEE_754-1985#Double-precision_64-bit
 */
struct obl_double_storage {

    /* Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /* Exponent, biased with +1023 (-1022 if denormalized). */
    unsigned int exponent :11;

    /* 1.mantissa in binary. */
    unsigned long long mantissa :52;

};

/*
 * Char
 *
 * Single unicode character (not a single code point).
 */
struct obl_char_storage {
    UChar32 value;
};

/*
 * String
 *
 * Length-prefixed UTF-16 string.
 */
struct obl_string_storage {

    /* Size of the string, in code points (not in characters). */
    obl_uint length;

    /* Array of code points. */
    UChar *contents;

};

/*
 * Boolean
 *
 * Truth or falsehood.  Boolean objects are never instantiated: rather, the
 * objects True and False reside at fixed logical addresses and are returned
 * from abstract I/O functions as needed.
 */
struct obl_boolean_storage {

    /* Truth is 1, Falsehood is 0. */
    obl_uint value;

};

/*
 * Nil
 *
 * The null object.  Like Booleans, nil is never instantiated directly, but has
 * one instance accessable at the logical address 0.  Actually, nil has no
 * internal storage at all: it's represented by a null pointer in the storage
 * field.
 */
struct obl_nil_storage {
    void *nothing;
};

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
struct obl_stub_storage {
    obl_logical_address value;
};

/*
 * A wrapper that contains an object's shape and internal storage.  Most
 * external and language binding code should work with obl_object objects
 * and use the functions provided here to manipulate them.
 */
struct obl_object
{

    /* Database that this object is stored in, or NULL. */
    struct obl_database *database;

    /* The logical address of this object, if one has been assigned. */
    obl_logical_address logical_address;

    /* The physical address within the database, if this instance is persisted. */
    obl_physical_address physical_address;

    /* Shape of this instance. */
    struct obl_object *shape;

    /* Internal data storage.  The active internal storage module is determined by
     * the +shape+ of the instance (NULL indicates shape storage).
     */
    union
    {
        struct obl_shape_storage *shape_storage;

        struct obl_slotted_storage *slotted_storage;
        struct obl_fixed_storage *fixed_storage;
        struct obl_chunk_storage *chunk_storage;
        struct obl_treepage_storage *treepage_storage;

        struct obl_integer_storage *integer_storage;
        struct obl_float_storage *float_storage;
        struct obl_double_storage *double_storage;

        struct obl_char_storage *char_storage;
        struct obl_string_storage *string_storage;

        struct obl_boolean_storage *boolean_storage;
        struct obl_nil_storage *nil_storage;
        struct obl_stub_storage *stub_storage;

        void *any_storage;
    } storage;
};

/*
 * ============================================================================
 * Translate primitive C types into their equivalent obl_object structures.
 * ============================================================================
 */

/* int to INTEGER object. */
struct obl_object *obl_create_integer(struct obl_database *d, obl_int i);

/* float to FLOAT object. */
struct obl_object *obl_create_float(struct obl_database *d, float f);

/* double to DOUBLE object. */
struct obl_object *obl_create_double(struct obl_database *d, double dbl);

/* char to CHAR object, including translation from US-ASCII to UTF-16. */
struct obl_object *obl_create_char(struct obl_database *d, char c);

/* Unicode UChar32 to CHAR object. */
struct obl_object *obl_create_uchar(struct obl_database *d, UChar32 uc);

/* Unicode string to STRING object. */
struct obl_object *obl_create_string(struct obl_database *d,
        const UChar *uc, obl_uint length);

/* C string to UTF-16 STRING object. */
struct obl_object *obl_create_cstring(struct obl_database *d,
        const char *c, obl_uint length);

/* Fixed-size collection creation. */
struct obl_object *obl_create_fixed(struct obl_database *d, obl_uint length);

/* Slotted object creation */
struct obl_object *obl_create_slotted(struct obl_object *shape);

/* Construction of SHAPE objects from individual shape components. */
struct obl_object *obl_create_shape(struct obl_database *d,
        struct obl_object *name, struct obl_object *slot_names,
        obl_storage_type type);

/*
 * Direct creation of SHAPE objects from C primitives, for convenience.  Shapes
 * created with this function *must* be destroyed with +obl_destroy_cshape+ to
 * deallocate internal objects.
 */
struct obl_object *obl_create_cshape(struct obl_database *d,
        char *name, size_t slot_count, char **slot_names,
        obl_storage_type type);

/*
 * ============================================================================
 * Common operations on shapes and built-in collection types.
 * ============================================================================
 */

/* FIXED objects */

/*
 * Return the number of elements present in a fixed-size collection +o+.
 */
uint32_t obl_fixed_size(const struct obl_object *fixed);

/*
 * Access an element of the fixed-size collection +o+ at the zero-based index
 * +index+.
 */
struct obl_object *obl_fixed_at(const struct obl_object *fixed,
        const obl_uint index);

/*
 * Set an element of the fixed-size collection +o+ at +index+ to point to the
 * object +value+.
 */
void obl_fixed_at_put(struct obl_object *fixed, const obl_uint index,
        struct obl_object *value);

/* STRING objects */

/* Return the number of code points contained in the STRING object +o+. */
obl_uint obl_string_size(const struct obl_object *string);

/*
 * Acquire at most +buffer_size+ US-ASCII characters into +buffer+.  Return the
 * number of code points copied.
 */
size_t obl_string_chars(const struct obl_object *string,
        char *buffer, size_t buffer_size);

/*
 * Return zero if the contents of +string_a+ exactly match those of +string_b+, or
 * nonzero if either is not a STRING or have different contents.
 */
int obl_string_cmp(const struct obl_object *string_a,
        const struct obl_object *string_b);

/*
 * Return zero if the contents of +o+ exactly match the NULL-terminated C string
 * +match+, nonzero otherwise.
 */
int obl_string_ccmp(const struct obl_object *string, const char *match);

/* SLOTTED objects */

/*
 * Return the object at an indexed slot of +slotted+.
 */
struct obl_object *obl_slotted_at(const struct obl_object *slotted,
        const obl_uint index);

/*
 * Return the contents of a slot by name.
 */
struct obl_object *obl_slotted_atnamed(const struct obl_object *slotted,
        const struct obl_object *slotname);

/*
 * Return the contents of a slot by name, specified by C string.
 */
struct obl_object *obl_slotted_atcnamed(const struct obl_object *slotted,
        const char *slotname);

/*
 * Set the value of a slot by index.
 */
void obl_slotted_at_put(struct obl_object *slotted, const obl_uint index,
        struct obl_object *value);

/*
 * Set the value of a slot by name.
 */
void obl_slotted_atnamed_put(struct obl_object *slotted,
        const struct obl_object *slotname, struct obl_object *value);

/*
 * Set the value of a slot by name, specified as a C string.
 */
void obl_slotted_atcnamed_put(struct obl_object *slotted,
        const char *slotname, struct obl_object *value);

/* SHAPE objects */

/*
 * Return the number of slots present in the shape +o+.
 */
obl_uint obl_shape_slotcount(const struct obl_object *shape);

/*
 * Return the index (zero-based) of a slot with a given name, or -1 if no slot
 * has that name.
 */
obl_uint obl_shape_slotnamed(const struct obl_object *shape,
        const struct obl_object *name);

/*
 * Convenience wrapper for +obl_shape_slotnamed+ that uses a C string.
 */
obl_uint obl_shape_slotcnamed(const struct obl_object *shape,
        const char *name);

/*
 * Accessor for the storage type of a SHAPE object.
 */
obl_storage_type obl_shape_storagetype(const struct obl_object *shape);

/*
 * ============================================================================
 * Translate struct obl_object structures into primitive C types.
 * ============================================================================
 */

/*
 * Return the stored value of an integer object +o+ as a C int.
 */
obl_int obl_integer_value(const struct obl_object *o);

/*
 * Convert the +obl_true()+ or +obl_false()+ objects into the appropriate truth
 * value for C if statements and so on.
 */
int obl_boolean_value(const struct obl_object *o);

/*
 * Acquire at most +buffer_size+ code points contained within a STRING object
 * +o+ into a prepared +buffer+.  Return the number of code points copied.
 */
size_t obl_string_value(const struct obl_object *o,
        UChar *buffer, size_t buffer_size);

/*
 * ============================================================================
 * Various methods of object destruction.
 * ============================================================================
 */

/*
 * Orderly struct obl_object deallocation.
 */
void obl_destroy_object(struct obl_object *o);

/*
 * Destroy full SHAPE objects, including slot names and shape name.  Created to
 * parallel +obl_create_cshape+.
 */
void obl_destroy_cshape(struct obl_object *o);

/*
 * ============================================================================
 * Private methods that should only be called elsewhere within ObjectLite.
 * ============================================================================
 */

/* Creates the one and only NIL instance in the database. */
struct obl_object *_obl_create_nil(struct obl_database *d);

/* Creates the only instances of true (1) and false (0). */
struct obl_object *_obl_create_bool(struct obl_database *d, int truth);

/* Placeholder for deferring an object load operation. */
struct obl_object *_obl_create_stub(struct obl_database *d,
        obl_logical_address address);

/*
 * Return the actual object a STUB is standing in for.  +depth+ controls
 * how far into the object graph other object references are resolved.
 */
struct obl_object *_obl_resolve_stub(struct obl_object *o, int depth);

/*
 * Return the storage type specified by +o+'s shape.
 */
obl_storage_type _obl_storage_of(const struct obl_object *o);

/* Returns true if +o+ is an object with STUB storage. */
int _obl_is_stub(struct obl_object *o);

#endif
