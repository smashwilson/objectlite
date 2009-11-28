/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file stub.h
 */

#ifndef STUB_H
#define STUB_H

#include "platform.h"

/* defined in object.h */
struct obl_object;

/* defined in database.h */
struct obl_database;

/* defined in session.h */
struct obl_session;

/**
 * Stand in for an object that has not yet been loaded.  The slots of objects
 * that are too deep in the object graph to load directly are instead populated
 * by psuedo-objects with stub storage.  Stubs contain only the logical address
 * of the real object.
 *
 * Client code should never see an obl_object with stub storage, because the
 * object access API resolves them as they are seen.
 */
struct obl_stub_storage {

    /** The address at which the actual, stubbed object can be located. */
    obl_logical_address value;

};

/** Placeholder for deferring an object load operation. */
struct obl_object *_obl_create_stub(struct obl_session *s,
        obl_logical_address address);

/**
 * If an object is a stub, return the actual object it's standing in for;
 * otherwise, return it untouched.
 *
 * @param o An object, possibly of stub storage.
 * @return Either o itself, or a new obl_object that it was deferring.
 */
struct obl_object *_obl_resolve_stub(struct obl_object *o);

/** Returns true if o is a stub. */
int _obl_is_stub(struct obl_object *o);

#endif /* STUB_H */
