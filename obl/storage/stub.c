/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/stub.h"

#include "storage/object.h"
#include "database.h"
#include "set.h"

#include <stdlib.h>

struct obl_object *_obl_create_stub(struct obl_database *d,
        obl_logical_address address)
{
    struct obl_object *result;
    struct obl_stub_storage *storage;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_stub_storage*)
            malloc(sizeof(struct obl_stub_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    storage->value = address;

    result->shape = obl_at_address(d, OBL_STUB_SHAPE_ADDR);
    result->storage.stub_storage = storage;
    result->logical_address = address;

    /*
     * Store the newly created stub within the read set, so that it will be
     * deallocated with the rest of the read objects.
     */
    obl_set_insert(d->read_set, result);

    return result;
}

struct obl_object *_obl_resolve_stub(struct obl_object *stub)
{
    if (_obl_is_stub(stub)) {
        return obl_at_address_depth(stub->database,
                stub->storage.stub_storage->value,
                stub->database->default_stub_depth);
    } else {
        return stub;
    }
}

int _obl_is_stub(struct obl_object *o)
{
    struct obl_object *shape;

    shape = o->shape;
    if (shape == obl_nil(o->database)) {
        return 0;
    } else {
        return obl_shape_storagetype(shape) == OBL_STUB;
    }
}
