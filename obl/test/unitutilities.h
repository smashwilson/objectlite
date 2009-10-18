/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Utility functions, macros, and so on that are useful through the unit-testing
 * suite.
 */

#ifndef UNITUTILITIES_H
#define UNITUTILITIES_H

/*
 * Register the test function +name+ with CUnit.  Use within an
 * +initialize_xx_suite+ function to cleanly register a test case.
 */
#define ADD_TEST(name) \
    if (CU_add_test(pSuite, #name, name) == NULL) {\
        return NULL;\
    }

/*
 * Print the contents of +memory+ in hexadecimal, char by char.  Useful to pop
 * in when a memcmp() is failing.
 */
void dump_memory(char *memory, int size, const char *filename);

#endif /* UNITUTILITIES_H */
