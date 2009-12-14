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
#include "session.h"
#include "set.h"

#include <stdlib.h>

struct obl_object *_obl_create_stub(struct obl_session *s,
        obl_logical_address address)
{
    struct obl_object *result;
    struct obl_stub_storage *storage;

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }

    storage = malloc(sizeof(struct obl_stub_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(s->database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    storage->value = address;

    result->shape = obl_at_address(s, OBL_STUB_SHAPE_ADDR);
    result->storage.stub_storage = storage;
    result->logical_address = address;
    result->session = s;

    return result;
}

struct obl_object *_obl_resolve_stub(struct obl_object *stub)
{
    if (_obl_is_stub(stub)) {
        return _obl_at_address_depth(stub->session,
                stub->storage.stub_storage->value,
                obl_database_of(stub)->configuration.default_stub_depth, 1);
    } else {
        return stub;
    }
}

int _obl_is_stub(struct obl_object *o)
{
    struct obl_object *shape;

    shape = o->shape;
    if (shape == obl_nil()) {
        return 0;
    } else {
        return obl_shape_storagetype(shape) == OBL_STUB;
    }
}
