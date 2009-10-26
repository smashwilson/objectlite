/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * A utility function to stress-test our obl_set implementation.  Unlike the
 * other unit tests, this one will only be executed if you manually call it,
 * mainly because it's a randomized test and has to be run for a good while
 * to be sure to test all of the different cases.
 */

#include "set.h"

#include "database.h"
#include "storage/object.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/**
 * Stress test an obl_set implementation.  Populates the set with arbitrary
 * data, then deletes an arbitrary subset of it, then validates the structure
 * of the resulting internal set representation.  This function will never
 * return; instead it will exit with a 0 or 1 status for success or failure.
 *
 * @param insert_count The number of objects to store in the tree.
 * @param delete_count The number of objects to remove from the tree.
 */
void stress_test_set(unsigned long insert_count,
        unsigned long delete_count)
{
    struct obl_set *set;
    struct obl_database *d;
    obl_logical_address addr;
    int black_height;

    srand(time(NULL));

    d = obl_create_database("foo.obl");
    set = obl_create_set(&logical_address_keyfunction);

    printf("Starting...");
    for (addr = 0; addr < insert_count; addr++) {
        struct obl_object *o;

        o = obl_create_integer(d, (obl_uint) addr);
        o->logical_address = (obl_logical_address) addr;

        obl_set_insert(set, o);
        if (addr % 1000 == 0) {
            printf("\rInserted <%8lu>", addr);
        }
    }
    printf("\n");

    for (addr = 0; addr < delete_count; addr++) {
        struct obl_object o;
        o.logical_address = (obl_logical_address) rand();

        obl_set_remove(set, &o);

        if (addr % 1000 == 0) {
            printf("\rDeleted  <%8lu>", addr);
        }
    }
    printf("\n");

    black_height = obl_set_verify(set);
    if (black_height == 0) {
        fprintf(stderr, "Set is invalid!");
        exit(1);
    } else {
        printf("Set is valid! Black height: %d\n", black_height);
        exit(0);
    }
}
