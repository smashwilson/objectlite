/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Cross-platform storage of single precision fixed point numeric quantities.
 */

#ifndef FLOAT_H
#define FLOAT_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * Fractional number, stored in 32 bits as sign bit, exponent and mantissa.
 * Follows the IEEE 754-1985 standard explicitly to ensure binary compatibility
 * across compilers and architectures.  Of course, this may or may not actually
 * correspond to a native C float for any particular build, so conversions
 * should be done with caution.
 *
 * The numeric value represented can be calculated as:
 *   (-1)^sign * 2^(exponent - 127) * (1 + mantissa)
 * If the number is normalized (exponent == 0 and mantissa != 0), or:
 *   (-1)^sign * 2^(-126) * (0 + mantissa)
 * if it is denormalized.  There are other special cases for signed 0s, NaN,
 * and the infinities which you can read about below.
 *
 * Reference: http://en.wikipedia.org/wiki/IEEE_754-1985#Single-precision_32-bit
 */
struct obl_float_storage {

    /** Sign bit.  0 indicates a positive value. */
    unsigned int sign :1;

    /** Exponent, biased with 127. */
    unsigned int exponent :8;

    /** 1.mantissa in binary. */
    unsigned int mantissa :23;

};

/**
 * Create a new floating-point object.
 *
 * \param d The database that should own this object.
 * \param f A C single-precision float to use as its value.
 * \return A newly allocated object containing obl_float_storage.
 */
struct obl_object *obl_create_float(struct obl_database *d, float f);

#endif /* FLOAT_H */
