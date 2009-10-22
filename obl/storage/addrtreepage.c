/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/addrtreepage.h"

#include "database.h"
#include "storage/object.h"

struct obl_object *obl_create_addrtreepage(struct obl_database *d,
        obl_uint depth)
{
    struct obl_object *result;
    struct obl_addrtreepage_storage *storage;
    obl_uint i;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }
    result->shape = obl_at_address(d, OBL_ADDRTREEPAGE_SHAPE_ADDR);

    storage = (struct obl_addrtreepage_storage*)
            malloc(sizeof(struct obl_addrtreepage_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    storage->height = depth;
    for (i = 0; i < CHUNK_SIZE; i++) {
        storage->contents[i] = OBL_PHYSICAL_UNASSIGNED;
    }
    result->storage.addrtreepage_storage = storage;

    return result;
}
