/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file transaction.c
 */

#include "transaction.h"

#include "database.h"
#include "session.h"
#include "set.h"

static void _deallocate_transaction(struct obl_transaction *t);

struct obl_transaction *obl_begin_transaction(struct obl_session *session)
{
    struct obl_transaction *t;

    if (session->current_transaction != NULL) {
        obl_report_error(session->database, OBL_ALREADY_IN_TRANSACTION, NULL);
        return NULL;
    }

    t = malloc(sizeof(struct obl_transaction));
    if (t == NULL) {
        obl_report_error(session->database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    t->write_set = obl_create_set(&logical_address_keyfunction);
    t->session = session;
    session->current_transaction = t;

    return t;
}

struct obl_transaction *obl_ensure_transaction(struct obl_session *session,
        int *created)
{
    if (session->current_transaction != NULL) {
        *created = 0;
        return session->current_transaction;
    } else {
        *created = 1;
        return obl_begin_transaction(session);
    }
}

void obl_mark_dirty(struct obl_object *o)
{
    struct obl_transaction *t;

    if (o->session == NULL ||
            o->session->current_transaction == NULL ||
            o->logical_address == OBL_LOGICAL_UNASSIGNED)
        return ;

    t = o->session->current_transaction;
    obl_set_insert(t->write_set, o);
}

int obl_commit_transaction(struct obl_transaction *t)
{
    struct obl_set_iterator *it = obl_set_destroying_iter(t->write_set);
    struct obl_object *current;

    while ( (current = obl_set_iternext(it)) != NULL ) {
        if (current->physical_address != OBL_PHYSICAL_UNASSIGNED) {
            _obl_write(current);
        }
    }

    _deallocate_transaction(t);
    return 0;
}

void obl_abort_transaction(struct obl_transaction *t)
{
    obl_destroy_set(t->write_set, NULL);
    _deallocate_transaction(t);
}

static void _deallocate_transaction(struct obl_transaction *t)
{
    t->session->current_transaction = NULL;
    free(t);
}
