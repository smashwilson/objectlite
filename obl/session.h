/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file session.h
 *
 * Defines sessions, a tool for interacting with obl_database stores in
 * multithreaded environments.
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

/**
 * An obl_session represents one thread or process' view of the data contained
 * within the obl_database.  Sessions cache object reads and manage writes with
 * transactions.
 */
struct obl_session
{
    /**
     * The database of which this session provides a view.
     */
    struct obl_database *database;

    /**
     * If non-NULL, this session has an active current transaction.
     */
    struct obl_transaction *current_transaction;

    /**
     * A red-black tree of all objects resident within the session, keyed by
     * logical address.
     */
    struct obl_set *read_set;

    /**
     * Semaphore to protect access to any of this session's resources.
     */
    sem_t session_mutex;
};

/**
 * A singly-linked list of obl_session objects.
 */
struct obl_session_list
{
    struct obl_session *entry;

    struct obl_session_list *next;
};

/**
 * Allocate and return a new obl_session.
 *
 * @param database An opened database.
 * @return A newly allocated obl_session structure.
 */
struct obl_session *obl_create_session(struct obl_database *database);

/**
 * Return one session's view of another session's object.
 *
 * @param s The session whose view of o you want.
 * @param o An object belonging to a session that isn't s.
 * @return A new object.
 */
struct obl_object *obl_in(struct obl_session *s, struct obl_object *o);

/**
 * The most basic query: return an object that lives at a known logical address.
 * Use the default stub depth as configured in the database.
 *
 * @param session
 * @param address
 * @return The obl_object within the database that has been assigned this
 *      address, or obl_nil() if no such object exists.
 */
struct obl_object *obl_at_address(struct obl_session *session,
        obl_logical_address address);

/**
 * Retrieve an object to a specified stub depth.
 *
 * @param session
 * @param address
 * @param depth Follow this many object references.
 * @return The obl_object at the root of the discovered object graph, or
 *      obl_nil() if no such object exists.
 */
struct obl_object *obl_at_address_depth(struct obl_session *session,
        obl_logical_address address, int depth);

/**
 * Re-read a persisted object from its native storage.
 *
 * @param o The object to refresh, which must be persisted by some session.
 */
void obl_refresh_object(struct obl_object *o);

/**
 * Deallocate a session and remove it from its owning database.
 *
 * @param session
 */
void obl_destroy_session(struct obl_session *session);

/**
 * Append an item onto the front of a session list.
 *
 * @param list [out] The session list to modify.
 * @param s The session to add.
 */
void obl_session_list_append(struct obl_session_list **list,
        struct obl_session *s);

/**
 * Remove any and all references to a certain session from the provided session
 * list.
 *
 * @param list [out] The session list to modify.  Note that this pointer may
 *      become NULL if the list becomes empty.
 * @param s The exact session to remove.
 */
void obl_session_list_remove(struct obl_session_list **list,
        struct obl_session *s);

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

/**
 * Use obl_refresh_object() to acquire a new version of each object contained
 * within a change set.  For internal use only.
 *
 * @param s
 * @param change_set
 */
void _obl_update_objects(struct obl_session *s,
        struct obl_set *change_set);

#endif /* SESSION_H */
