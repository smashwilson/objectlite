/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Boolean objects contain a single truth or falsehood value.  In practice,
 * only two boolean storage objects are created, true and false; they reside
 * within fixed space at OBL_TRUE_ADDR and OBL_FALSE_ADDR, respectively.
 */

#ifndef BOOLEAN_H
#define BOOLEAN_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * Truth or falsehood.  Boolean objects are never instantiated: rather, the
 * objects True and False reside at fixed logical addresses and are returned
 * from abstract I/O functions as needed.
 */
struct obl_boolean_storage {

    /** For consistency with C conventions, Truth is 1, Falsehood is 0. */
    obl_uint value;

};

/**
 * Creates the only instances of true (1) and false (0).  For internal use
 * only.
 *
 * \param d The database to create this boolean object within.
 * \param truth 1 creates the object true, 0 creates the object false.  Other
 *      values will also create the object true.
 * \return A newly allocated obl_object containing obl_boolean_storage.
 *
 * \sa obl_true()
 * \sa obl_false()
 */
struct obl_object *_obl_create_bool(struct obl_database *d, int truth);

/**
 * Convert the +obl_true()+ or +obl_false()+ objects into the appropriate truth
 * value for C if statements and so on.
 *
 * \param o An object with OBL_BOOLEAN storage.
 * \return 1 if o is true, 0 if it is false.  Reports an error and returns true
 *      if o is not a boolean.
 */
int obl_boolean_value(struct obl_object *o);

/**
 * Output a boolean object to stdout.
 *
 * \param boolean The boolean object.
 * \param depth Unused.
 * \param indent The level of indentation.
 */
void obl_print_boolean(struct obl_object *boolean, int depth, int indent);

#endif /* BOOLEAN_H */
