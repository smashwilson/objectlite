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

struct obl_session
{
    struct obl_database *database;

    struct obl_transaction *current_transaction;

    sem_t session_lock;
};

struct obl_session *obl_create_session(struct obl_database *database);

/**
 * The most basic query: return an object that lives at a known logical address.
 * Use the default stub depth as configured in the database.
 */
struct obl_object *obl_at_address(struct obl_session *session,
        const obl_logical_address address);

/**
 * Retrieve an object to a specified stub depth.
 */
struct obl_object *obl_at_address_depth(struct obl_session *session,
        const obl_logical_address address, int depth);

void obl_destroy_session(struct obl_session *session);

#endif /* SESSION_H */
