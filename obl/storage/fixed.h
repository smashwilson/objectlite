/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * A position-indexed collection of obl_object instances with immutable size.
 * Fixed collections provide efficient storage for small, largely static
 * sets of objects, such as instance variable names or set indices.
 */

#ifndef FIXED_H
#define FIXED_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * An immutable length collection containing position-indexed references to
 * other objects.
 *
 * Fixed storage is inappropriate for large (hundreds of elements, say)
 * collections or for collections that are frequently resized.  Use
 * obl_chunk_storage for such situations.
 */
struct obl_fixed_storage {

    /* The size of the collection. */
    obl_uint length;

    /* Collection payload. */
    struct obl_object **contents;

};

/**
 * Create a new fixed-size collection.
 *
 * \param d The database that should own this object.
 * \param length The number of elements this collection should accomodate.
 * \return A newly allocated obl_object with obl_fixed_storage and all elements
 *      initialized as obl_nil().
 */
struct obl_object *obl_create_fixed(struct obl_database *d,
        obl_uint length);

/**
 * Access the number of elements present in a fixed-size collection.
 *
 * \param fixed An object with fixed storage.
 * \return The number of elements contained within fixed.  Produces an error and
 *      returns 0 if fixed does not have fixed storage.
 */
obl_uint obl_fixed_size(const struct obl_object *fixed);

/**
 * Access an individual element of a fixed-size collection.
 *
 * \param fixed An object with fixed storage.
 * \param index The zero-based index of the element to retrieve.
 * \return The obl_object currently at the provided position.  Reports an error
 *      if index is out of valid access bounds for this collection, or if
 *      fixed does not actually have fixed storage.
 */
struct obl_object *obl_fixed_at(const struct obl_object *fixed,
        const obl_uint index);

/**
 * Set an element of a fixed-size collection.
 *
 * \param fixed An object with fixed storage.
 * \param index The zero-based index to place this object at.
 * \param value The object to assign to that position.
 * \return The obl_object currently at the provided position.  Reports an error
 *      if index is out of valid access bounds for this collection, or if
 *      fixed does not actually have fixed storage.
 */
void obl_fixed_at_put(struct obl_object *fixed, const obl_uint index,
        struct obl_object *value);

/**
 * Output the contents of a fixed collection to stdout.
 *
 * \param fixed An object with fixed storage.
 * \param depth Used to control object graph recursion.
 * \param indent The level of output indentation.
 */
void obl_print_fixed(struct obl_object *fixed, int depth, int indent);

#endif /* FIXED_H */
