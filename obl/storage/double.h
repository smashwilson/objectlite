/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Cross-platform storage for IEEE double precision floating-point numeric
 * values.
 */

#ifndef DOUBLE_H
#define DOUBLE_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * One double-precision floating point number, stored in 64 bits.  Storage is
 * similar to the single-precision Float with a longer exponent and mantissa.
 *
 * Reference: http://en.wikipedia.org/wiki/IEEE_754-1985#Double-precision_64-bit
 */
struct obl_double_storage {

    /* Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /* Exponent, biased with +1023 (-1022 if denormalized). */
    unsigned int exponent :11;

    /* 1.mantissa in binary. */
    unsigned long long mantissa :52;

};

/**
 * Convert a native C double into an obl_object with obl_double_storage.
 *
 * \param d The database to create this object within.
 * \param dbl A C double-precision value.
 * \return The newly allocated obl_object.
 */
struct obl_object *obl_create_double(struct obl_database *d, double dbl);

#endif /* DOUBLE_H */
