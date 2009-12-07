/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Utility functions, macros, and so on that are useful through the unit-testing
 * suite.
 */

#ifndef UNITUTILITIES_H
#define UNITUTILITIES_H

/* defined in database.h */
struct obl_database *d;

/**
 * Register the test function +name+ with CUnit.  Use within an
 * +initialize_xx_suite+ function to cleanly register a test case.
 */
#define ADD_TEST(name) \
    if (CU_add_test(pSuite, #name, name) == NULL) {\
        return NULL;\
    }

#define BASE 100

/**
 * Set a word's worth of char slots within the content of a database to the
 * specified values.
 */
#define SET_CHAR(mem, addr, zero, one, two, three) \
        ((char*) (mem))[(addr) * sizeof(obl_uint)] = zero;\
        ((char*) (mem))[(addr) * sizeof(obl_uint) + 1] = one;\
        ((char*) (mem))[(addr) * sizeof(obl_uint) + 2] = two;\
        ((char*) (mem))[(addr) * sizeof(obl_uint) + 3] = three

/**
 * Set a full obl_uint value within the content of a database.
 */
#define SET_UINT(mem, addr, value) \
    (mem)[addr] = writable_uint(value)

/**
 * Zero out the contents of the database.
 *
 * @param d
 */
void wipe(struct obl_database *d);

/**
 * Print the contents of +memory+ in hexadecimal, char by char.  Useful to pop
 * in when a memcmp() is failing.
 */
void dump_memory(char *memory, int size, const char *filename);

/**
 * Write the full contents of a database to the specified filename.
 */
void dump(struct obl_database *d, const char *filename);

#endif /* UNITUTILITIES_H */
