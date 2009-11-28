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

int obl_commit_transaction(struct obl_transaction *transaction)
{
    _deallocate_transaction(transaction);
    return 0;
}

void obl_abort_transaction(struct obl_transaction *transaction)
{
    _deallocate_transaction(transaction);
}

static void _deallocate_transaction(struct obl_transaction *t)
{
    t->session->current_transaction = NULL;

    obl_destroy_set(t->write_set, NULL);
    free(t);
}
