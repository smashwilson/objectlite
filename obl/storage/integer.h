/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Cross platform storage for signed integer storage.  On most platforms, this
 * corresponds to the native int or long type.
 */

#ifndef INTEGER_H
#define INTEGER_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * A signed integer value within the range +/- 2^32 - 1.
 */
struct obl_integer_storage {

    obl_int value;

};

/**
 * Convert a C int to an OBL_INTEGER object.
 *
 * \param d The database that should own this integer.
 * \param i An integral value.
 * \return A newly allocated obl_object containing the provided value.
 */
struct obl_object *obl_create_integer(struct obl_database *d, obl_int i);

/**
 * Modify the stored value of an existing OBL_INTEGER object.  Produces an
 * error if integer is not actually an integer object.
 *
 * \param integer An object with integer storage.
 * \param value The new value for integer to assume.
 */
void obl_integer_set(struct obl_object *integer, obl_int value);

/**
 * Return the stored value of an integer object as a C int.  Produces an error
 * and returns 0 if o is not actually an integer.
 *
 * \param o An object with integer storage.
 */
obl_int obl_integer_value(struct obl_object *o);

/**
 * Output an integer to stdout.
 *
 * \param integer An object with integer storage.
 * \param depth Unused.
 * \param indent The level of output indentation.
 */
void obl_print_integer(struct obl_object *integer, int depth, int indent);

#endif /* INTEGER_H */
