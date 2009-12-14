/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Storage for the most common type of user-created class: those that have a
 * small, fixed number of named slots, or instance variables.
 */

#ifndef SLOTTED_H
#define SLOTTED_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/* defined in session.h */
struct obl_session;

/**
 * Contains zero to many "slots" (more commonly known as instance variables).
 * Each slot contains a reference to another object.  The number of slots and
 * the names of each slot are specified by the object's shape.
 */
struct obl_slotted_storage {

    /**
     * An array of object references, mapped to slot_names by position.  The
     * array size is determined by the number of slot names defined in the
     * object's shape.
     */
    struct obl_object **slots;

};

/**
 * Creation of slotted objects.
 *
 * \param shape The shape to use for this object.
 * \return A newly allocated slotted object.  Reports an error if shape is
 *      not a valid shape.
 */
struct obl_object *obl_create_slotted(struct obl_object *shape);

/**
 * Return the object at a slot by index.
 *
 * \param slotted An object with slotted storage.
 * \param index The zero-based slot index.
 * \return The obl_object currently referenced by the requested slot.  Reports
 *      an error and returns obl_nil() if slotted is not actually a slotted
 *      object, or if index is out of the legal range.
 */
struct obl_object *obl_slotted_at(struct obl_object *slotted,
        obl_uint index);

/**
 * Return the contents of a slot by name.
 *
 * \param slotted An object with slotted storage.
 * \param slotname An obl_string_storage object containing the name of the slot
 *      to query.
 * \return The obl_object currently referenced by the named slot.  Reports an
 *      an error and returns obl_nil() if slotted is not actually a slotted
 *      object, or if its shape contains no slot with that name.
 */
struct obl_object *obl_slotted_atnamed(struct obl_object *slotted,
        struct obl_object *slotname);

/**
 * Return the contents of a slot by name, specified by C string.
 *
 * \sa obl_slotted_atnamed
 * \sa obl_create_string
 */
struct obl_object *obl_slotted_atcnamed(struct obl_object *slotted,
        const char *slotname);

/**
 * Set the value of a slot by index.  Reports an error if index is out of
 * bounds.
 */
void obl_slotted_at_put(struct obl_object *slotted, obl_uint index,
        struct obl_object *value);

/**
 * Set the value of a slot by name.
 */
void obl_slotted_atnamed_put(struct obl_object *slotted,
        struct obl_object *slotname, struct obl_object *value);

/**
 * Set the value of a slot by name, specified as a C string.
 *
 * \sa obl_slotted_atnamed_put
 */
void obl_slotted_atcnamed_put(struct obl_object *slotted,
        const char *slotname, struct obl_object *value);

/**
 * Read a slotted object.  The number of slots expected is determined by the
 * provided shape.
 */
struct obl_object *obl_slotted_read(struct obl_session *session,
        struct obl_object *shape, obl_uint *source,
        obl_physical_address offset, int depth);

/**
 * Write a slotted object.
 */
void obl_slotted_write(struct obl_object *slotted, obl_uint *dest);

/**
 * Output a slotted object nicely to stdout.
 *
 * \param slotted A slotted object.
 * \param depth Used to control object graph recursion.
 * \param indent The level of output indentation.
 */
void obl_slotted_print(struct obl_object *slotted, int depth, int indent);

/**
 * Iterate over a slotted object's referenced children.  For internal use
 * only; does not resolve stubs.
 *
 * @param slotted The root object.
 * @return An obl_object_list containing this object's immediate references.
 */
struct obl_object_list *_obl_slotted_children(struct obl_object *slotted);

/**
 * Deallocate a slotted-storage object.  For internal use only.
 *
 * \param slotted An object with slotted storage.
 */
void _obl_slotted_deallocate(struct obl_object *slotted);

#endif /* SLOTTED_H */
