/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file session.c
 */

#include "session.h"

#include <stdlib.h>

#include "storage/object.h"
#include "addressmap.h"
#include "set.h"
#include "transaction.h"
#include "database.h"

struct obl_session *obl_create_session(struct obl_database *database)
{
    if (database == NULL) {
        OBL_ERROR(database,
                "Attempt to create a session without a valid database.");
        return NULL;
    }

    struct obl_session *session = malloc(sizeof(struct obl_session));

    if (session == NULL) {
        obl_report_error(database, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    session->database = database;

    session->read_set = obl_create_set(&logical_address_keyfunction);
    session->current_transaction = NULL;

    sem_init(&session->session_mutex, 0, 1);

    sem_wait(&database->session_list_mutex);
    obl_session_list_append(&database->session_list, session);
    sem_post(&database->session_list_mutex);

    return session;
}

struct obl_object *obl_in(struct obl_session *s, struct obl_object *o)
{
    if (o->session == s)
        return o;
    return obl_at_address(s, o->logical_address);
}

struct obl_object *obl_at_address(struct obl_session *session,
        obl_logical_address address)
{
    return obl_at_address_depth(session, address,
            session->database->configuration.default_stub_depth);
}

struct obl_object *obl_at_address_depth(struct obl_session *session,
        obl_logical_address address, int depth)
{
    return _obl_at_address_depth(session, address, depth, 1);
}

void obl_refresh_object(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_database *d = s->database;
    struct obl_object *n;

    sem_wait(&d->content_mutex);
    sem_wait(&s->session_mutex);
    n = obl_read_object(s, d->content, o->physical_address,
            d->configuration.default_stub_depth);

    o->shape = n->shape;
    o->storage.any_storage = n->storage.any_storage;
    sem_post(&s->session_mutex);
    sem_post(&d->content_mutex);

    /*
     * Free n directly; its storage is now referenced by o.
     */
    free(n);
}

void obl_destroy_session(struct obl_session *session)
{
    struct obl_database *d = session->database;

    if (session->current_transaction != NULL) {
        obl_abort_transaction(session->current_transaction);
    }

    obl_destroy_set(session->read_set, &_obl_deallocate_object);

    sem_destroy(&session->session_mutex);

    sem_wait(&d->session_list_mutex);
    obl_session_list_remove(&d->session_list, session);
    sem_post(&d->session_list_mutex);

    free(session);
}

void obl_session_list_append(struct obl_session_list **list,
        struct obl_session *s)
{
    struct obl_session_list *node;

    node = malloc(sizeof(struct obl_session_list));
    node->entry = s;
    node->next = *list;

    *list = node;
}

void obl_session_list_remove(struct obl_session_list **list,
        struct obl_session *s)
{
    struct obl_session_list *current = *list, *previous = NULL;

    while (current != NULL) {
        struct obl_session_list *next = current->next;

        if (current->entry == s) {
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                *list = current->next;
            }

            free(current);
        }

        current = next;
    }
}

void _obl_session_release(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_transaction *t;

    if (s == NULL) return;
    sem_wait(&s->session_mutex);

    obl_set_remove(s->read_set, o);

    t = s->current_transaction;
    if (t != NULL) {
        obl_set_remove(t->write_set, o);
    }

    sem_post(&s->session_mutex);
}

struct obl_object *_obl_at_address_depth(struct obl_session *s,
        obl_logical_address address, int depth, int top)
{
    struct obl_database *d = s->database;
    struct obl_object *o;
    obl_physical_address physical;

    /* Check within fixed address space first. */
    if (IS_FIXED_ADDR(address)) {
        return _obl_at_fixed_address(address);
    }

    /* If this object already exists within the read set, return it as-is. */
    if (top) sem_wait(&s->session_mutex);
    o = obl_set_lookup(s->read_set, (obl_set_key) address);
    if (o != NULL && ! _obl_is_stub(o)) {
        if (top) sem_post(&s->session_mutex);
        return o;
    }

    if (depth > 0) {
        /* Look up the physical address. */
        physical = obl_address_lookup(d, address);
        if (physical == OBL_PHYSICAL_UNASSIGNED) {
            if (top) sem_post(&s->session_mutex);
            return obl_nil();
        }

        o = obl_read_object(s, d->content, physical, depth);
        o->logical_address = address;
        o->session = s;
    } else {
        /* Create and return a stub that will resolve to this object. */
        o = _obl_create_stub(s, address);
    }

    obl_set_insert(s->read_set, o);
    if (top) sem_post(&s->session_mutex);

    return o;

}

void _obl_update_objects(struct obl_session *s, struct obl_set *change_set)
{
    struct obl_set_iterator *it;
    struct obl_object *current, *mine;

    it = obl_set_inorder_iter(change_set);

    sem_wait(&s->session_mutex);
    while ( (current = obl_set_iternext(it)) != NULL ) {
        mine = obl_set_lookup(s->read_set,
                (obl_set_key) current->logical_address);
        if (mine != NULL) {
            obl_refresh_object(mine);
        }
    }
    sem_post(&s->session_mutex);
}
