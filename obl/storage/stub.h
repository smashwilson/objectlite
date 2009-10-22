/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#ifndef STUB_H
#define STUB_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/*
 * Stub
 *
 * Stand in for an object that has not yet been loaded.  The slots of objects
 * that are too deep in the object graph to load directly are instead populated
 * by psuedo-objects with stub storage.  Stubs contain only the logical address
 * of the real object.
 *
 * Client code should never see an obl_object with stub storage, because the
 * object access API resolves them as they are seen.
 */
struct obl_stub_storage {

    obl_logical_address value;

};

/* Placeholder for deferring an object load operation. */
struct obl_object *_obl_create_stub(struct obl_database *d,
        obl_logical_address address);

/*
 * Return the actual object a STUB is standing in for.  +depth+ controls
 * how far into the object graph other object references are resolved.
 */
struct obl_object *_obl_resolve_stub(struct obl_object *o, int depth);

/* Returns true if +o+ is an object with STUB storage. */
int _obl_is_stub(struct obl_object *o);

#endif /* STUB_H */
