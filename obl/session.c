/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file session.c
 */

#include "session.h"

#include <stdlib.h>

#include "storage/object.h"
#include "addressmap.h"
#include "set.h"
#include "transaction.h"
#include "database.h"

struct obl_session *obl_create_session(struct obl_database *database)
{
    if (database == NULL) {
        OBL_ERROR(database,
                "Attempt to create a session without a valid database.");
        return NULL;
    }

    struct obl_session *session = malloc(sizeof(struct obl_session));

    if (session == NULL) {
        obl_report_error(database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    session->database = database;

    session->read_set = obl_create_set(&logical_address_keyfunction);
    session->current_transaction = NULL;

    sem_init(&session->session_mutex, 0, 1);

    return session;
}

struct obl_object *obl_at_address(struct obl_session *session,
        obl_logical_address address)
{
    return obl_at_address_depth(session, address,
            session->database->configuration.default_stub_depth);
}

struct obl_object *obl_at_address_depth(struct obl_session *session,
        obl_logical_address address, int depth)
{
    return _obl_at_address_depth(session, address, depth, 1);
}

void obl_destroy_session(struct obl_session *session)
{
    if (session->current_transaction != NULL) {
        obl_abort_transaction(session->current_transaction);
    }

    obl_destroy_set(session->read_set, &_obl_deallocate_object);

    sem_destroy(&session->session_mutex);

    free(session);
}

void _obl_session_release(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_transaction *t;

    if (s == NULL) return;
    sem_wait(&s->session_mutex);

    obl_set_remove(s->read_set, o);

    t = s->current_transaction;
    if (t != NULL) {
        obl_set_remove(t->write_set, o);
    }

    sem_post(&s->session_mutex);
}

struct obl_object *_obl_at_address_depth(struct obl_session *s,
        obl_logical_address address, int depth, int top)
{
    struct obl_database *d = s->database;
    struct obl_object *o;
    obl_physical_address physical;

    /* Check within fixed address space first. */
    if (IS_FIXED_ADDR(address)) {
        return _obl_at_fixed_address(address);
    }

    /* If this object already exists within the read set, return it as-is. */
    if (top) sem_wait(&s->session_mutex);
    o = obl_set_lookup(s->read_set, (obl_set_key) address);
    if (o != NULL && ! _obl_is_stub(o)) {
        if (top) sem_post(&s->session_mutex);
        return o;
    }

    if (depth > 0) {
        /* Look up the physical address. */
        physical = obl_address_lookup(d, address);
        if (physical == OBL_PHYSICAL_UNASSIGNED) {
            if (top) sem_post(&s->session_mutex);
            return obl_nil();
        }

        o = obl_read_object(s, d->content, physical, depth);
        o->logical_address = address;
        o->session = s;
    } else {
        /* Create and return a stub that will resolve to this object. */
        o = _obl_create_stub(s, address);
    }

    obl_set_insert(s->read_set, o);
    if (top) sem_post(&s->session_mutex);

    return o;

}
