/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file transaction.h
 */

#ifndef TRANSACTION_H
#define TRANSACTION_H

/* Defined in set.h */
struct obl_set;

struct obl_transaction
{
    struct obl_set *write_set;
};

struct obl_transaction *obl_begin_transaction(struct obl_session *session);

struct obl_transaction *obl_ensure_transaction(struct obl_session *session,
        int *created);

int obl_commit_transaction(struct obl_transaction *transaction);

void obl_abort_transaction(struct obl_transaction *transaction);

#endif /* TRANSACTION_H */
