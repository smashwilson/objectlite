/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * The nothing object, used to populate unassigned slots in slotted objects,
 * fixed objects, or chunk objects.  Only one nil object should actually be
 * created by ObjectLite per obl_database; it is accessible via obl_nil().
 */

#ifndef NIL_H
#define NIL_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * The null object.  Like Booleans, nil is never instantiated directly, but has
 * one instance accessable at the logical address OBL_NIL_ADDR.  Actually, nil
 * has no internal storage at all: it's represented by a null pointer in the
 * storage field.
 */
struct obl_nil_storage {
    void *nothing;
};

/**
 * Creates the one and only NIL instance in the database.  For internal use
 * only.
 *
 * \param d The database that should own this nil object.
 */
struct obl_object *_obl_create_nil(struct obl_database *d);

/**
 * Output nil to stdout.
 *
 * \param nil The nil object.
 * \param depth Ignored.
 * \param indent The level of output indentation.
 */
void obl_nil_print(struct obl_object *nil, int depth, int indent);

#endif /* NIL_H */
