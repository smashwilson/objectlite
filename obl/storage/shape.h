/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Every object in an ObjectLite store has exactly one shape, which describes
 * its storage.
 */

#ifndef SHAPE_H
#define SHAPE_H

#include "storage/storagetypes.h"
#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * A "Class" object which specifies how to interpret any object whose header
 * word points to it.
 */
struct obl_shape_storage {

    /**
     * The shape's name, including a language-agnostic namespace prefix.  This
     * field is used as a key within an obl_database's shape map.  Must be an
     * object with obl_string_storage.
     */
    struct obl_object *name;

    /**
     * A fixed-size collection of slot names in the order that they will occur
     * within instances.  Must be an object with obl_fixed_storage.
     */
    struct obl_object *slot_names;

    /**
     * If non-nil, specifies the migration destination for instances of this
     * shape.  Instances will be migrated to this shape on read and persisted in
     * their new shape on write.  Must be another object of obl_shape_storage.
     */
    struct obl_object *current_shape;

    /**
     * The internal storage format to be used for I/O of instances of this
     * shape.  Must be a valid value of obl_storage_type.
     */
    obl_uint storage_format;

};

/**
 * Construction of a shape object from already-persisted members.
 *
 * \param d The database that should own this shape.
 * \param name A language-agnostic representation of this shape's fully-
 *      qualified name.  Must be unique among all shapes known by the database.
 * \param slot_names A fixed-size collection of obl_string_storage objects
 *      that name each slot within instances of this shape.
 * \param type The storage type to be used by this shape.
 */
struct obl_object *obl_create_shape(struct obl_database *d,
        struct obl_object *name, struct obl_object *slot_names,
        enum obl_storage_type type);

/**
 * Direct creation of SHAPE objects from C primitives, for convenience.  Shapes
 * created with this function *must* be destroyed with obl_destroy_cshape() to
 * deallocate internal objects.
 *
 * \param d The database that should own this shape.
 * \param name A C string containing this shape's database unique, language
 *      agnostic fully-qualified name.
 * \param slot_count The number of elements within slot_names.
 * \param slot_names An array of C strings naming each slot to be contained
 *      by instances of this shape, in order.
 * \param type The storage type used by this shape.
 */
struct obl_object *obl_create_cshape(struct obl_database *d,
        char *name, size_t slot_count, char **slot_names,
        enum obl_storage_type type);

/**
 * Return the name object of a shape.
 */
struct obl_object *obl_shape_name(struct obl_object *shape);

/**
 * Return the fixed collection of a shape's slot names.
 */
struct obl_object *obl_shape_slotnames(struct obl_object *shape);

/**
 * Return the number of slots present in the shape.
 *
 * \param shape The shape object to query.
 * \return The number of slots contained within shape.  Reports an error and
 *      returns 0 if shape is not actually a shape.
 */
obl_uint obl_shape_slotcount(struct obl_object *shape);

/**
 * Return the index of a slot with a given name.
 *
 * \param shape The shape object.
 * \param name An object with obl_string_storage.
 * \return The zero-based index of a slot with the given name.  Returns
 *      OBL_SENTINEL if no slot has that name.  Reports an error if shape is
 *      not a valid shape.
 */
obl_uint obl_shape_slotnamed(struct obl_object *shape,
        struct obl_object *name);

/**
 * Convenience wrapper for obl_shape_slotnamed() that accepts a C string.
 */
obl_uint obl_shape_slotcnamed(struct obl_object *shape,
        const char *name);

/**
 * Return the current migration destination of a shape.
 */
struct obl_object *obl_shape_currentshape(struct obl_object *shape);

/**
 * Accessor for the storage type of a shape.
 */
enum obl_storage_type obl_shape_storagetype(struct obl_object *shape);

/**
 * Destroy full shape objects, including slot names and shape name.  Created to
 * parallel obl_create_cshape().
 */
void obl_destroy_cshape(struct obl_object *o);

/**
 * Read a shape object.  Shapes are themselves a fixed shape (sorry, no turtles
 * all the way down -- yet).
 */
struct obl_object *obl_read_shape(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/**
 * Write a shape object.
 */
void obl_write_shape(struct obl_object *string, obl_uint *dest);

/**
 * Output a shape nicely to stdout.
 *
 * \param shape A shape object.
 * \param depth Used to control object graph recursion.
 * \param indent The level of output indentation.
 */
void obl_print_shape(struct obl_object *shape, int depth, int indent);

/**
 * Assign results to an array of the obl_object instances reachable from this
 * one.
 *
 * \param shape The shape object.
 * \param results [out] Assigned to the array address.
 * \param heaped [out] True; we have to heap allocate results.
 * \return The number of valid references in results.
 */
obl_uint _obl_shape_children(struct obl_object *shape,
        struct obl_object **results, int *heaped);

#endif /* SHAPE_H */
