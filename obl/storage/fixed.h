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

    /** The size of the collection. */
    obl_uint length;

    /** Collection payload. */
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
obl_uint obl_fixed_size(struct obl_object *fixed);

/**
 * Access an individual element of a fixed-size collection.
 *
 * \param fixed An object with fixed storage.
 * \param index The zero-based index of the element to retrieve.
 * \return The obl_object currently at the provided position.  Reports an error
 *      if index is out of valid access bounds for this collection, or if
 *      fixed does not actually have fixed storage.
 */
struct obl_object *obl_fixed_at(struct obl_object *fixed, obl_uint index);

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
void obl_fixed_at_put(struct obl_object *fixed, obl_uint index,
        struct obl_object *value);

/**
 * Read a fixed-length collection.
 */
struct obl_object *obl_read_fixed(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/**
 * Write a fixed-length collection.
 */
void obl_write_fixed(struct obl_object *fixed, obl_uint *dest);

/**
 * Output the contents of a fixed collection to stdout.
 *
 * \param fixed An object with fixed storage.
 * \param depth Used to control object graph recursion.
 * \param indent The level of output indentation.
 */
void obl_print_fixed(struct obl_object *fixed, int depth, int indent);

/**
 * Provide access to the obl_objects recursively referenced by this one.  Beware,
 * this function does not resolve stubs; for internal use only.
 *
 * \param fixed The root object.
 * \param results [out] Pointed to the first obl_object child.
 * \param heaped [out] False; fixed objects already have a nice heap array
 *      to return as it is.
 * \return The number of array-indexable obl_object structures following
 *      results.
 */
obl_uint _obl_fixed_children(struct obl_object *fixed,
        struct obl_object **results, int *heaped);

/**
 * Deallocate a fixed object, its internal storage, and any obl_stub_storage
 * objects linked from it.  For internal use only.
 *
 * \param fixed The object to delete.
 */
void _obl_fixed_deallocate(struct obl_object *fixed);

#endif /* FIXED_H */
