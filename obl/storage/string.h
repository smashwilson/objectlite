/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Cross-platform, international storage of string objects, encoded uniformly
 * and compactly as UTF-16BE.
 */

#ifndef STRING_H
#define STRING_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * A length-prefixed UTF-16 string.
 */
struct obl_string_storage {

    /** Size of the string, in code points (not in characters). */
    obl_uint length;

    /** Array of code points. */
    UChar *contents;

};

/**
 * Create a new obl_string_storage object from a Unicode string.
 *
 * @param uc A Unicode string, stored as a C array of code points.
 * @param length The length of uc.
 * @return The newly allocated obl_string_storage object.
 */
struct obl_object *obl_create_string(struct obl_database *d,
        const UChar *uc, obl_uint length);

/**
 * A convenience method to convert a small C string into an obl_object.
 *
 * @param c A C string.
 * @param length The length of c.
 */
struct obl_object *obl_create_cstring(struct obl_database *d,
        const char *c, obl_uint length);

/**
 * Return the number of code points contained within a string object.  Notice
 * that this is *not* the number of characters in the string if it contains
 * certain letters, accents, or other extended marks.
 */
obl_uint obl_string_size(struct obl_object *string);

/**
 * Convert an object's obl_string_storage into C native US-ASCII characters.
 *
 * \param string An object with string storage.
 * \param buffer [out] A character buffer of sufficient size in which the result
 *      should be stored.
 * \param buffer_size The allocated size of buffer.  No more than this many
 *      characters will be copied.  As a general rule, this will be no more
 *      than double the number of code points, as reported by obl_string_size().
 * \return The number of characters successfully copied.
 *
 * \sa obl_string_value
 */
size_t obl_string_chars(struct obl_object *string,
        char *buffer, size_t buffer_size);

/**
 * Return zero if the contents of string_a exactly match those of string_b, or
 * nonzero if either is not a STRING or have different contents.
 */
int obl_string_cmp(struct obl_object *string_a,
        struct obl_object *string_b);

/**
 * Return zero if the contents of string exactly match the NULL-terminated C
 * string match, nonzero otherwise.
 */
int obl_string_ccmp(struct obl_object *string, const char *match);

/**
 * Acquire at most buffer_size code points contained within an
 * obl_string_storage object o into a prepared buffer.  Return the number of
 * code points copied.
 *
 * \sa obl_string_chars
 */
size_t obl_string_value(struct obl_object *o,
        UChar *buffer, size_t buffer_size);

/**
 * Read a length-prefixed UTF-16BE string object.
 */
struct obl_object *obl_string_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Write a string object.
 */
void obl_string_write(struct obl_object *string, obl_uint *dest);

/**
 * Output a string to stdout.
 *
 * \param string A string object.
 * \param depth Ignored.
 * \param indent The level of output indentation.
 */
void obl_string_print(struct obl_object *string, int depth, int indent);

/**
 * Destroy a string's internal storage.  For internal use only.
 *
 * \param string The string to destroy.
 */
void _obl_string_deallocate(struct obl_object *string);

/**
 * Allocate the common internal structure of a string object.  For internal
 * use only.
 *
 * @param uc A mutable, heap-allocated array of code points it should contain.
 * @param length The length of uc.
 * @return The newly allocated obl_object.
 */
struct obl_object *_allocate_string(struct obl_database *d,
        UChar *uc, obl_uint length);

#endif /* STRING_H */
