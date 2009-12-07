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

#include <stdlib.h>

static struct obl_transaction *_allocate_transaction(
        struct obl_session *s);

static void _deallocate_transaction(struct obl_transaction *t);

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
    if (s->current_transaction == NULL ||
            o->logical_address == OBL_LOGICAL_UNASSIGNED) {
        sem_post(&s->session_mutex);
        return ;
    }

    t = s->current_transaction;
    obl_set_insert(t->write_set, o);

    sem_post(&s->session_mutex);
}

int obl_commit_transaction(struct obl_transaction *t)
{
    struct obl_set_iterator *it;
    struct obl_object *current;
    struct obl_session *s = t->session;
    struct obl_database *d = s->database;

    sem_wait(&d->content_mutex);
    sem_wait(&s->session_mutex);

    it = obl_set_destroying_iter(t->write_set);
    while ( (current = obl_set_iternext(it)) != NULL ) {
        if (current->physical_address != OBL_PHYSICAL_UNASSIGNED) {
            _obl_write(current);
        }
    }
    obl_set_destroyiter(it);
    _deallocate_transaction(t);

    sem_post(&s->session_mutex);
    sem_post(&d->content_mutex);

    return 0;
}

void obl_abort_transaction(struct obl_transaction *t)
{
    struct obl_session *s = t->session;

    sem_wait(&s->session_mutex);

    obl_destroy_set(t->write_set, NULL);
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
