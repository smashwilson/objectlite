/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the the public in-memory representation of objects as they are stored
 * within, and retrieved from, ObjectLite databases.
 *
 * object.h is also the master include for all available storage types; that is,
 * including object.h will allow you direct access to all of the object
 * manipulation functions for all storage types.
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "storage/storagetypes.h"
#include "platform.h"

/* Include the headers for every storage type. */
#include "storage/addrtreepage.h"
#include "storage/boolean.h"
#include "storage/char.h"
#include "storage/chunk.h"
#include "storage/double.h"
#include "storage/fixed.h"
#include "storage/float.h"
#include "storage/integer.h"
#include "storage/nil.h"
#include "storage/shape.h"
#include "storage/slotted.h"
#include "storage/string.h"
#include "storage/stub.h"

/* Defined in database.h */
struct obl_database;

/**
 * The structure that contains an object's shape and internal storage.  Most
 * external and language binding code should work with obl_object objects
 * and use the functions provided in storage/ headers to manipulate them.
 */
struct obl_object
{

    /** Database that this object is stored in. */
    struct obl_database *database;

    /**
     * The logical address of this object, if one has been assigned, or
     * OBL_LOGICAL_UNASSIGNED if one has not.
     */
    obl_logical_address logical_address;

    /**
     * The physical address within the database, if this instance is persisted,
     * or OBL_PHYSICAL_UNASSIGNED if it is not.
     */
    obl_physical_address physical_address;

    /** The shape of this instance. */
    struct obl_object *shape;

    /**
     * Internal data storage.  The active internal storage module is determined by
     * the shape of the instance (obl_nil() indicates shape storage).
     */
    union
    {
        struct obl_shape_storage *shape_storage;

        struct obl_slotted_storage *slotted_storage;
        struct obl_fixed_storage *fixed_storage;
        struct obl_chunk_storage *chunk_storage;
        struct obl_addrtreepage_storage *addrtreepage_storage;

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

/**
 * Return the (non-recursive) storage size of this object.  Includes the shape
 * word in its calculations.
 *
 * \param o The object to size.
 * \return The storage size of o, in units of sizeof(obl_uint).
 */
obl_uint obl_object_wordsize(struct obl_object *o);

/**
 * Access the current shape of an object.
 */
struct obl_object *obl_object_shape(struct obl_object *o);

/**
 * Output the contents of an arbitrary object to stdout in a storage-defined
 * way.
 *
 * \param o The object to output.
 * \param depth How far to recurse into the object graph.  Notice that no
 *      cycle detection is done during the printing process.
 * \param indent The base indentation level.
 */
void obl_print_object(struct obl_object *o, int depth, int indent);

/**
 * Orderly obl_object deallocation.  Frees both the object itself and its
 * internal storage.
 *
 * \param o The object to destroy.
 */
void obl_destroy_object(struct obl_object *o);

/**
 * Return the storage type of an object.
 *
 * \param o The object to inspect.
 * \return The storage type of the object o, as acquired from its current
 *      shape.
 */
enum obl_storage_type obl_storage_of(const struct obl_object *o);

/**
 * Allocate a new obl_object from the heap, without specified storage.  For
 * internal use only.
 *
 * \param d The obl_database that should own this object.
 * \return An obl_object if the malloc is successful, or NULL if it is not.  An
 *      error will be logged in d if the allocation is unsuccessful.
 */
struct obl_object *_obl_allocate_object(struct obl_database *d);

#endif
