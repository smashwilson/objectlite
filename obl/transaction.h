/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file transaction.h
 *
 * Transactions provide a orderly way to apply changes to the objects stored
 * within a database in well-defined, atomic steps.
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

/**
 * A transaction contains state that will be applied if it is committed or
 * discarded if it is aborted.
 */
struct obl_transaction
{
    /** The session that owns this transaction. */
    struct obl_session *session;

    /**
     * Objects that have been changed while this transaction has been active.
     */
    struct obl_set *write_set;
};

/**
 * Allocate a new transaction and mark it as the current one within a session.
 *
 * @param session
 * @return A newly allocated obl_transaction.
 */
struct obl_transaction *obl_begin_transaction(struct obl_session *session);

/**
 * If the session have an active transaction already, return it.  Otherwise,
 * begin a new transaction, and set created to true.
 *
 * @param session
 * @param created [out] Will be set to 0 if a transaction already exists or 1
 *      if one needed to be created.
 * @return An active, valid transaction.
 */
struct obl_transaction *obl_ensure_transaction(struct obl_session *session,
        int *created);

/**
 * If o is a persisted object, and its session has an active transaction, add
 * o to its write set.
 *
 * @param o
 */
void obl_mark_dirty(struct obl_object *o);

/**
 * Apply any and all object changes recorded within a transaction.  Discover
 * and persist any unpersisted objects that are now referenced by persisted
 * ones, assigning addresses and session ownership as necessary.  Notify
 * other sessions within the same database that reference any changed
 * objects to acquire the latest object data.  Destroy the transaction object.
 *
 * @param transaction This memory will be freed before the call returns.
 */
int obl_commit_transaction(struct obl_transaction *transaction);

/**
 * Revert any object changes recorded within a transaction.  Destroy the
 * transaction object.
 *
 * @param transaction This memory will be freed before the call returns.
 */
void obl_abort_transaction(struct obl_transaction *transaction);

#endif /* TRANSACTION_H */
