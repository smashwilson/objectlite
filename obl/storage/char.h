/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Storage for single-character objects.  Supports any single Unicode character.
 * Provided in addition to obl_string_storage for the benefit of those languages
 * that distinguish between characters and strings of length 1.
 */

#ifndef CHAR_H
#define CHAR_H

#include "platform.h"

#include "unicode/utypes.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * A single Unicode character (not a single code point).
 */
struct obl_char_storage {

    UChar32 value;

};

/**
 * Create an obl_object from a native C char.  Automatically converts into
 * UTF-32.
 *
 * TODO Stub
 *
 * \param d The database that should own this object.
 * \param c A US-ASCII encoded C character.
 * \return A newly allocated obl_object with obl_char_storage.
 */
struct obl_object *obl_create_char(struct obl_database *d, char c);

/**
 * Create an obl_object directly from a Unicode character.
 *
 * TODO Stub
 *
 * \param d The database that should own this object.
 * \param uc Any Unicode character.
 * \return A newly allocated obl_object with obl_char_storage.
 */
struct obl_object *obl_create_uchar(struct obl_database *d, UChar32 uc);

/**
 * Print a character object to stdout.
 *
 * TODO Stub
 *
 * \param c An object with character storage.
 * \param depth Determines the level of indentation.
 */
void obl_print_char(struct obl_object *c, int depth);

#endif /* CHAR_H */
