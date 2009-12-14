/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file transaction.c
 */

#include "transaction.h"

#include "addressmap.h"
#include "allocator.h"
#include "database.h"
#include "session.h"
#include "set.h"

#include <stdlib.h>

/* Internal function prototypes. */

static struct obl_transaction *_allocate_transaction(
        struct obl_session *s);

static void _deallocate_transaction(struct obl_transaction *t);

/**
 * Recursively visit the transitive closure of a root object, assigning
 * any missing logical or physical addresses or session references.  The
 * traversal is stubbed where addresses or session references already exist
 * (and hence the traversal will not be an infinite loop).
 *
 * @param s The session to adopt objects on behalf of.
 * @param o The current root of traversal.
 * @param adopted [out] A list of all objects that needed session references
 *      and/or addresses.
 */
static void _visit_transitive_closure(struct obl_session *s,
        struct obl_object *o, struct obl_object_list **adopted);

/* External function definitions. */

struct obl_transaction *obl_begin_transaction(struct obl_session *s)
{
    struct obl_transaction *t;

    sem_wait(&s->session_mutex);
    t = _allocate_transaction(s);
    sem_post(&s->session_mutex);

    return t;
}

struct obl_transaction *obl_ensure_transaction(struct obl_session *s,
        int *created)
{
    struct obl_transaction *t;

    if (s == NULL)
        return NULL;

    sem_wait(&s->session_mutex);
    if (s->current_transaction != NULL) {
        *created = 0;
        t = s->current_transaction;
    } else {
        *created = 1;
        t = _allocate_transaction(s);
    }
    sem_post(&s->session_mutex);

    return t;
}

void obl_mark_dirty(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_transaction *t;

    if (s == NULL) return ;

    sem_wait(&s->session_mutex);
    if (s->current_transaction == NULL) {
        sem_post(&s->session_mutex);
        return ;
    }

    t = s->current_transaction;
    obl_set_insert(t->write_set, o);

    sem_post(&s->session_mutex);
}

int obl_commit_transaction(struct obl_transaction *t)
{
    struct obl_set_iterator *scan_it, *write_it;
    struct obl_object *current;
    struct obl_object_list *adopted = NULL;
    struct obl_session *s = t->session;
    struct obl_database *d = s->database;
    unsigned long count = 0, adopt_count = 0;

    OBL_DEBUG(d, "Beginning commit.");

    sem_wait(&d->content_mutex);
    sem_wait(&s->session_mutex);

    /*
     * Scan all objects in the write set for references to any nonpersisted
     * obl_objects.  Assign them to this transaction's session and accumulate
     * them into the obl_object_list adopted.
     */
    scan_it = obl_set_inorder_iter(t->write_set);
    while ( (current = obl_set_iternext(scan_it)) != NULL ) {
        _visit_transitive_closure(s, current, &adopted);
    }
    obl_set_destroyiter(scan_it);

    /*
     * Now that they have addresses and so on, add all adopted objects to the
     * transaction write set and the session's read set.
     */
    while (adopted != NULL) {
        struct obl_object_list *former;

        current = adopted->entry;
        obl_set_insert(t->write_set, current);
        obl_set_insert(s->read_set, current);
        adopt_count++;

        former = adopted;
        adopted = adopted->next;
        free(former);
    }

    /*
     * Write each dirty object to the database.
     */
    write_it = obl_set_destroying_iter(t->write_set);
    while ( (current = obl_set_iternext(write_it)) != NULL ) {
        count++;
        _obl_write(current);
    }
    obl_set_destroyiter(write_it);

    /*
     * Destroy this transaction and remove it from the session.  Its work
     * is now complete.
     */
    _deallocate_transaction(t);

    sem_post(&s->session_mutex);
    sem_post(&d->content_mutex);

    OBL_DEBUGF(d,
            "Successful commit of %lu objects, "
            "%lu previously unpersisted.", count, adopt_count);

    return 0;
}

void obl_abort_transaction(struct obl_transaction *t)
{
    struct obl_session *s = t->session;
    struct obl_set_iterator *iter;
    struct obl_object *current;

    sem_wait(&s->session_mutex);

    iter = obl_set_destroying_iter(t->write_set);
    while ( (current = obl_set_iternext(iter)) != NULL ) {
        obl_refresh_object(current);
    }
    obl_set_destroyiter(iter);

    _deallocate_transaction(t);

    sem_post(&s->session_mutex);
}

static void _deallocate_transaction(struct obl_transaction *t)
{
    t->session->current_transaction = NULL;
    free(t);
}

static struct obl_transaction *_allocate_transaction(
        struct obl_session *s)
{
    struct obl_transaction *t;

    if (s->current_transaction != NULL) {
        obl_report_error(s->database, OBL_ALREADY_IN_TRANSACTION, NULL);
        return NULL;
    }

    t = malloc(sizeof(struct obl_transaction));
    if (t == NULL) {
        obl_report_error(s->database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    t->write_set = obl_create_set(&logical_address_keyfunction);
    t->session = s;

    s->current_transaction = t;

    return t;
}

static void _visit_transitive_closure(struct obl_session *s,
        struct obl_object *o, struct obl_object_list **adopted)
{
    struct obl_object_list *children = NULL;
    int was_adopted = 0;

    if (o->session == NULL) {
        o->session = s;
        was_adopted = 1;
    }

    if (_obl_assign_addresses(o)) {
        /* o did not have a logical address. */
        was_adopted = 1;
    }

    if (was_adopted == 1) {
        obl_object_list_append(adopted, o);
    }

    /*
     * Recursively call with the children of this object that have no session
     * set or addresses.
     */
    children = _obl_children(o);

    while (children != NULL) {
        struct obl_object *current = children->entry;
        struct obl_object_list *former;

        if (! IS_FIXED_ADDR(current->logical_address) &&
                current->session == NULL) {
            _visit_transitive_closure(s, current, adopted);
        }

        former = children;
        children = children->next;
        free(former);
    }
}
