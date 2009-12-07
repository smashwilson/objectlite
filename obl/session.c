/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file session.c
 */

#include <stdlib.h>

#include "storage/object.h"
#include "session.h"
#include "set.h"
#include "transaction.h"
#include "database.h"

struct obl_session *obl_create_session(struct obl_database *database)
{
    struct obl_session *session = malloc(sizeof(struct obl_session));

    if (session == NULL) {
        obl_report_error(database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    session->database = database;
    session->current_transaction = NULL;
    sem_init(&session->lock, 0, 1);

    return session;
}

struct obl_object *obl_at_address(struct obl_session *session,
        const obl_logical_address address)
{
    return obl_at_address_depth(session, address,
            session->database->configuration.default_stub_depth);
}

struct obl_object *obl_at_address_depth(struct obl_session *session,
        const obl_logical_address address, int depth)
{
    return _obl_at_address_depth(session->database, session, address, depth);
}

void obl_destroy_session(struct obl_session *session)
{
    if (session->current_transaction != NULL) {
        obl_abort_transaction(session->current_transaction);
    }

    sem_destroy(&session->lock);
    free(session);
}

void _obl_session_release(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_transaction *t;

    if (s == NULL) return;
    t = s->current_transaction;
    if (t == NULL) return;

    sem_wait(&s->lock);
    obl_set_remove(t->write_set, o);
    sem_post(&s->lock);
}
