/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in
 * ObjectLite.
 */

#include "storage/object.h"

#include "database.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "unicode/ucnv.h"

/* Static function prototypes. */

static struct obl_object *invalid_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

static void invalid_write(struct obl_object *o, obl_uint *dest);

static void invalid_print(struct obl_object *o,
        int depth, int indent);

static obl_uint no_children(struct obl_object *root,
        struct obl_object **results, int *done);

static void simple_deallocate(struct obl_object *o);

/* Function types. */

/**
 * Signature of a function that reads and populates an object's internal state.
 * Each such function is provided the shape determined from the shape word, the
 * memory mapped to the database file, and an offset at which reading is to
 * occur.
 */
typedef struct obl_object *(*read_function)(
        struct obl_object *shape, obl_uint *source,
        obl_physical_address offset, int depth);

/**
 * Signature of a function that writes an obl_object into a memory-mapped file,
 * not including its shape header word.  The object will be written at a
 * location specified by its assigned physical address.
 */
typedef void (*write_function)(
        struct obl_object *object, obl_uint *source);

/**
 * Signature of a function that recursively prints an object to stdout.
 */
typedef void (*print_function)(struct obl_object *o,
        int depth, int indent);

/**
 * Signature of a function that allows iteration over an obl_object's references
 * to other obl_object structures.
 */
typedef obl_uint (*children_function)(struct obl_object *root,
        struct obl_object **results, int *heaped);

/**
 * Signature of a function that deallocates an object and its internal
 * structure.
 */
typedef void (*deallocate_function)(struct obl_object *o);

/* Function maps. */

/**
 * The array of object-reading functions: one for each internal state
 * specified in object.h and at the same index as its index in the
 * +obl_storage_type+ enumeration.
 */
static read_function read_functions[OBL_STORAGE_TYPE_MAX + 1] = {
        &obl_read_shape,        /* OBL_SHAPE */
        &obl_read_slotted,      /* OBL_SLOTTED */
        &invalid_read,          /* OBL_FIXED */
        &invalid_read,          /* OBL_CHUNK */
        &obl_read_addrtreepage, /* OBL_ADDRTREEPAGE */
        &obl_read_integer,      /* OBL_INTEGER */
        &invalid_read,          /* OBL_FLOAT */
        &invalid_read,          /* OBL_DOUBLE */
        &invalid_read,          /* OBL_CHAR */
        &obl_read_string,       /* OBL_STRING */
        &invalid_read,          /* OBL_BOOLEAN (invalid) */
        &invalid_read,          /* OBL_NIL (invalid) */
        &invalid_read           /* OBL_STUB (invalid) */
};

/**
 * The array of object-writing functions.  Each function serializes an object
 * of a certain storage type, not including its header byte word, to the
 * location specified by its pre-set physical address.
 */
static write_function write_functions[] = {
        &obl_write_shape,        /* OBL_SHAPE */
        &obl_write_slotted,      /* OBL_SLOTTED */
        &obl_write_fixed,        /* OBL_FIXED */
        &invalid_write,          /* OBL_CHUNK */
        &obl_write_addrtreepage, /* OBL_ADDRTREEPAGE */
        &obl_write_integer,      /* OBL_INTEGER */
        &invalid_write,          /* OBL_FLOAT */
        &invalid_write,          /* OBL_DOUBLE */
        &invalid_write,          /* OBL_CHAR */
        &obl_write_string,       /* OBL_STRING */
        &invalid_write,          /* OBL_BOOLEAN (invalid) */
        &invalid_write,          /* OBL_NIL (invalid) */
        &invalid_write           /* OBL_STUB (invalid) */
};

static print_function print_functions[OBL_STORAGE_TYPE_MAX + 1] = {
        &obl_print_shape,        /* OBL_SHAPE */
        &obl_print_slotted,      /* OBL_SLOTTED */
        &obl_print_fixed,        /* OBL_FIXED */
        &invalid_print,          /* OBL_CHUNK */
        &obl_print_addrtreepage, /* OBL_ADDRTREEPAGE */
        &obl_print_integer,      /* OBL_INTEGER */
        &invalid_print,          /* OBL_FLOAT */
        &invalid_print,          /* OBL_DOUBLE */
        &invalid_print,          /* OBL_CHAR */
        &obl_print_string,       /* OBL_STRING */
        &obl_print_boolean,      /* OBL_BOOLEAN */
        &obl_print_nil,          /* OBL_NIL */
        &invalid_print           /* OBL_STUB (invalid) */
};

static children_function children_functions[OBL_STORAGE_TYPE_MAX + 1] = {
        &_obl_shape_children,   /* OBL_SHAPE */
        &_obl_slotted_children, /* OBL_SLOTTED */
        &_obl_fixed_children,   /* OBL_FIXED */
        &no_children,           /* OBL_CHUNK */
        &no_children,           /* OBL_ADDRTREEPAGE */
        &no_children,           /* OBL_INTEGER */
        &no_children,           /* OBL_FLOAT */
        &no_children,           /* OBL_DOUBLE */
        &no_children,           /* OBL_CHAR */
        &no_children,           /* OBL_STRING */
        &no_children,           /* OBL_BOOLEAN */
        &no_children,           /* OBL_NIL */
        &no_children            /* OBL_STUB */
};

static deallocate_function deallocate_functions[OBL_STORAGE_TYPE_MAX + 1] = {
        &simple_deallocate,       /* OBL_SHAPE */
        &_obl_slotted_deallocate, /* OBL_SLOTTED */
        &_obl_fixed_deallocate,   /* OBL_FIXED */
        &simple_deallocate,       /* OBL_CHUNK */
        &simple_deallocate,       /* OBL_ADDRTREEPAGE */
        &simple_deallocate,       /* OBL_INTEGER */
        &simple_deallocate,       /* OBL_FLOAT */
        &simple_deallocate,       /* OBL_DOUBLE */
        &simple_deallocate,       /* OBL_CHAR */
        &_obl_string_deallocate,  /* OBL_STRING */
        &simple_deallocate,       /* OBL_BOOLEAN */
        &simple_deallocate,       /* OBL_NIL */
        &simple_deallocate        /* OBL_STUB */
};

/* Implementation. */

/* TODO use storage dispatch for this. */
obl_uint obl_object_wordsize(struct obl_object *o)
{
    switch(obl_storage_of(o)) {
    case OBL_SHAPE:
        return 5;
    case OBL_SLOTTED:
        return 1 + obl_shape_slotcount(o->shape);
    case OBL_FIXED:
        return 1 + obl_fixed_size(o);
    case OBL_CHUNK:
        return 2 + CHUNK_SIZE;
    case OBL_ADDRTREEPAGE:
        return 2 + CHUNK_SIZE;
    case OBL_INTEGER:
        return 2;
    case OBL_FLOAT:
        return 1 + (obl_uint) ceil((double) 32 / sizeof(obl_uint));
    case OBL_DOUBLE:
        return 1 + (obl_uint) ceil((double) 64 / sizeof(obl_uint));
    case OBL_CHAR:
        return 1 + (obl_uint) ceil((double) sizeof(obl_uint) / sizeof(UChar32));
    case OBL_STRING:
        return 1 + obl_string_size(o) *
            (obl_uint) ceil((double) sizeof(obl_uint) / sizeof(UChar));
    case OBL_BOOLEAN:
        return 2;
    case OBL_NIL:
        return 2;
    default:
        obl_report_error(o->database, OBL_WRONG_STORAGE,
                "obl_object_wordsize called with an object of unknown storage.");
        return 0;
    }
}

/* Objects shapes should never be stubbed. */
struct obl_object *obl_object_shape(struct obl_object *o)
{
    return o->shape;
}

void obl_print_object(struct obl_object *o, int depth, int indent)
{
    (print_functions[(int) obl_storage_of(o)])(o, depth, indent);
}

void obl_destroy_object(struct obl_object *o)
{
    if (o->storage.any_storage != NULL ) {
        free(o->storage.any_storage);
    }
    free(o);
}

enum obl_storage_type obl_storage_of(struct obl_object *o)
{
    struct obl_object *shape;

    shape = obl_object_shape(o);
    if (shape == obl_nil(o->database)) {
        return OBL_SHAPE;
    } else {
        return obl_shape_storagetype(shape);
    }
}

struct obl_object *obl_read_object(struct obl_database *d,
        obl_uint *source, obl_physical_address base, int depth)
{
    struct obl_object *shape, *result;
    obl_logical_address addr;
    int function_index;

    addr = (obl_logical_address) readable_uint(source[base]);
    shape = obl_at_address_depth(d, addr, 1);

    if (shape != obl_nil(d) && obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_errorf(d, OBL_WRONG_STORAGE,
                "Corrupt shape header at physical address %ul.",
                base);
        return obl_nil(d);
    }

    if (shape == obl_nil(d)) {
        function_index = OBL_SHAPE;
    } else {
        function_index = obl_shape_storagetype(shape);
    }

    result = (read_functions[function_index])(
            shape, source, base, depth);
    result->shape = shape;
    result->physical_address = base;

    return result;
}

/*
 * Writes the shape address and delegates to the appropriate write function
 * for this object's storage type.
 */
void obl_write_object(struct obl_object *o, obl_uint *dest)
{
    struct obl_object *shape;
    int function_index;

    shape = obl_object_shape(o);

    if (shape != obl_nil(o->database) && obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(o->database, OBL_WRONG_STORAGE,
                "Attempt to write an object with a shape that isn't a shape.");
        return ;
    }

    if (shape != obl_nil(o->database)) {
        function_index = (int) obl_shape_storagetype(shape);
    } else {
        function_index = (int) OBL_SHAPE;
    }

    dest[o->physical_address] = writable_logical(shape->logical_address);

    (*write_functions[function_index])(o, dest);
}

obl_uint _obl_children(struct obl_object *root,
        struct obl_object **results, int *heaped)
{
    *heaped = 0;
    return (*children_functions[obl_storage_of(root)])(root, results, heaped);
}

struct obl_object *_obl_allocate_object(struct obl_database *d)
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

/*
 * Delegate to one of the per-storage functions in deallocate_functions.  You
 * always want to free the pointer you're passed, so do that at the end.
 */
void _obl_deallocate_object(struct obl_object *o)
{
    (*deallocate_functions[obl_storage_of(o)])(o);
    free(o);
}

/* Static function implementations. */

/**
 * Invoked for any storage type that is either not defined yet, or isn't
 * supposed to actually be read from the database.
 */
static struct obl_object *invalid_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address base, int depth)
{
    obl_report_errorf(shape->database, OBL_WRONG_STORAGE,
            "Attempt to read an object (0x%04lx) with an invalid storage type.",
            base);
    return obl_nil(shape->database);
}

/**
 * Invoked for any storage type that is either not defined yet, or isn't
 * supposed to actually be written to the database.
 */
static void invalid_write(struct obl_object *o, obl_uint *dest)
{
    obl_report_errorf(o->database, OBL_WRONG_STORAGE,
            "Attempt to write an object with an invalid storage type (%lu).",
            obl_storage_of(o));
}

/**
 * Invoked for any storage type that is either invalid or whose print function
 * hasn't been written yet.
 */
static void invalid_print(struct obl_object *o,
        int depth, int indent)
{
    int in;

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("<INVALID: logical 0x%08lx physical 0x%08lx>",
            (unsigned long) o->logical_address,
            (unsigned long) o->physical_address);
}

/**
 * Invoked for any storage type that has no children (i.e. most of them).
 *
 * \param root Any object of a storage type without direct children.
 * \param results [out] Set to NULL.
 * \param heaped [out] Set to false.
 * \return Zero.
 */
static obl_uint no_children(struct obl_object *root,
        struct obl_object **results, int *heaped)
{
    *results = NULL;
    return (obl_uint) 0;
}

/**
 * An deallocate_function to be invoked for any object without
 * special storage requirements.  This is sufficient for any obl_object storage
 * type that does not perform malloc() calls in its creation function other
 * than the one within _obl_allocate_object() and the one for the storage
 * itself.
 *
 * \param o The object to deallocate.
 */
static void simple_deallocate(struct obl_object *o)
{
    if (o->storage.any_storage != NULL) {
        free(o->storage.any_storage);
    }
}
