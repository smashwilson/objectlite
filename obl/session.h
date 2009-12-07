/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file session.h
 */

#ifndef SESSION_H
#define SESSION_H

#include "platform.h"

/* Defined in database.h */
struct obl_database;

/* Defined in transaction.h */
struct obl_transaction;

/* Defined in set.h */
struct obl_set;

struct obl_session
{
    struct obl_database *database;

    struct obl_transaction *current_transaction;

    struct obl_set *read_set;

    sem_t session_mutex;
};

struct obl_session *obl_create_session(struct obl_database *database);

/**
 * The most basic query: return an object that lives at a known logical address.
 * Use the default stub depth as configured in the database.
 */
struct obl_object *obl_at_address(struct obl_session *session,
        obl_logical_address address);

/**
 * Retrieve an object to a specified stub depth.
 */
struct obl_object *obl_at_address_depth(struct obl_session *session,
        obl_logical_address address, int depth);

void obl_destroy_session(struct obl_session *session);

/**
 * Atomically release an object from any internal session data structures.
 *
 * @param o
 *
 * @sa obl_destroy_object()
 */
void _obl_session_release(struct obl_object *o);

/**
 * Primitive function used for actual database access.  Used in recursive calls
 * to prevent waiting on a mutex you're already holding.  For internal use
 * only.  Seriously, you could really screw up your database if you call this
 * with the wrong parameters.
 *
 * @param session
 * @param address
 * @param depth Object fault depth.
 * @param top If nonzero, session semaphores will be locked and unlocked.  If
 *      zero, ensuring thread safety is the caller's responsibility.
 *
 * @sa obl_at_address_depth()
 */
struct obl_object *_obl_at_address_depth(struct obl_session *session,
        obl_logical_address address, int depth, int top);

#endif /* SESSION_H */
