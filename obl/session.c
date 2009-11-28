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
    sem_init(&(session->session_lock), 0, 1);

    return session;
}

struct obl_object *obl_at_address(struct obl_session *session,
        const obl_logical_address address)
{
    return obl_at_address_depth(session, address,
            session->database->default_stub_depth);
}

struct obl_object *obl_at_address_depth(struct obl_session *session,
        const obl_logical_address address, int depth)
{
    return _obl_at_address_depth(session->database, session, address, depth);
}

void obl_destroy_session(struct obl_session *session)
{
    sem_destroy(&(session->session_lock));
    free(session);
}
