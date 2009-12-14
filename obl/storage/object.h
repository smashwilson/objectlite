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

/* Defined in session.h */
struct obl_session;

/**
 * The structure that contains an object's shape and internal storage.  Most
 * external and language binding code should work with obl_object objects
 * and use the functions provided in storage/ headers to manipulate them.
 */
struct obl_object
{
    /**
     * The database session that currently owns this object.  NULL if the
     * object has not yet been persisted.
     */
    struct obl_session *session;

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
 * A singly-linked list for passing around small collections of objects.  If
 * you have more than a few, consider using an obl_set.
 */
struct obl_object_list
{
    struct obl_object *entry;

    struct obl_object_list *next;
};

/**
 * Return the (non-recursive) storage size of this object.  Includes the shape
 * word in its calculations.
 *
 * @param o The object to size.
 * @return The storage size of o, in units of sizeof(obl_uint).
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
 * @param o The object to output.
 * @param depth How far to recurse into the object graph.  Notice that no
 *      cycle detection is done during the printing process.
 * @param indent The base indentation level.
 */
void obl_print_object(struct obl_object *o, int depth, int indent);

/**
 * Orderly obl_object deallocation.  Removes the object from any read or write
 * sets it's a member of, then destroys it.  This function should be invoked
 * whenever an object that shadows the obl_object in a language binding is
 * garbage collected.
 *
 * @param o The object to destroy.
 */
void obl_destroy_object(struct obl_object *o);

/**
 * Return the storage type of an object.
 *
 * @param o The object to inspect.
 * @return The storage type of the object o, as acquired from its current
 *      shape.
 */
enum obl_storage_type obl_storage_of(struct obl_object *o);

/**
 * Access the database to which an object belongs.
 *
 * @param o The object to query.
 * @return The obl_database in which this object is (or will be) persisted.
 */
struct obl_database *obl_database_of(struct obl_object *o);

/**
 * Read a shape word from the current position of the file `source`, retrieve
 * the shape object, then invoke the appropriate struct read_function from
 * the obl_read_functions table to read the rest of the object.  Return the
 * populated struct obl_object structure.
 *
 * The created object lives on the heap and must be destroyed with a call to
 * obl_destroy_object() as defined in object.h.
 *
 * @param s The session that should own this object.
 * @param source Binary contents to be interpreted.
 * @param offset The address into <code>source</code> that contains the
 *      shape header.
 * @param depth The number of references to follow into the object graph.
 * @return An object initialized from the data found at <code>source</code>.
 */
struct obl_object *obl_read_object(struct obl_session *s, obl_uint *source,
        obl_physical_address offset, int depth);

/**
 * Write an object to the memory-mapped file provided.  The object must
 * already have a physical and logical address assigned to it.
 */
void obl_write_object(struct obl_object *o, obl_uint *dest);

/**
 * Append an entry to an existing obl_object_list or create a new
 * obl_object_list.
 *
 * @param list [inout] NULL or an existing obl_object_list.
 * @param o The object to append.
 */
void obl_object_list_append(struct obl_object_list **list,
        struct obl_object *o);

/**
 * Deallocate a full obl_object_list.
 *
 * @param list The list to deallocate.
 */
void obl_destroy_object_list(struct obl_object_list *list);

/**
 * Useful for iterating over referenced obl_object structures.  For internal
 * use only; this call does not resolve any stubs encountered.
 *
 * @param root
 * @return An obl_object_list containing the immediate children of root.
 *      Returns NULL if root has no children.
 */
struct obl_object_list *_obl_children(struct obl_object *root);

/**
 * Allocate a new obl_object from the heap, without specified storage.  For
 * internal use only.
 *
 * @return An obl_object if the malloc is successful, or NULL if it is not.  An
 *      error will be logged if the allocation is unsuccessful.
 */
struct obl_object *_obl_allocate_object();

/**
 * Deallocate the memory associated with an obl_object.  For internal use only.
 * Properly disposes of internal structure, but does not recursively delete
 * an object's children.
 *
 * @param o The object to free.
 */
void _obl_deallocate_object(struct obl_object *o);

#endif
