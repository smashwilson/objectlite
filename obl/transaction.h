/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file transaction.h
 */

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "platform.h"

/* defined in set.h */
struct obl_set;

/* defined in session.h */
struct obl_session;

/* defined in storage/object.h */
struct obl_object;

struct obl_transaction
{
    struct obl_session *session;

    struct obl_set *write_set;
};

struct obl_transaction *obl_begin_transaction(struct obl_session *session);

struct obl_transaction *obl_ensure_transaction(struct obl_session *session,
        int *created);

void obl_mark_dirty(struct obl_object *o);

int obl_commit_transaction(struct obl_transaction *transaction);

void obl_abort_transaction(struct obl_transaction *transaction);

#endif /* TRANSACTION_H */
