/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Chunks provide reasonably efficient storage for variable-length collections
 * of other obl_objects.
 *
 * TODO Implement chunk creation and access.
 */

#ifndef CHUNK_H
#define CHUNK_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/**
 * Single section of a variable-length, position-indexed collection.  Chunks act
 * as a singly-linked list of nodes that contain batches of CHUNK_SIZE
 * consecutive collection elements.
 *
 * Chunk storage is wildly inefficient for small (fewer than a hundred element,
 * say) or immutable-length collections.  Use obl_fixed_storage for such cases
 * instead.
 */
struct obl_chunk_storage {

    /** The next chunk in the list. */
    struct obl_object *next;

    /** The contents of this chunk. */
    struct obl_object *contents[CHUNK_SIZE];

};

#endif /* CHUNK_H */
