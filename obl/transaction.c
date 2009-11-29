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
#include <stdio.h>

static struct obl_transaction *_allocate_transaction(
        struct obl_session *s);

static void _deallocate_transaction(struct obl_transaction *t);

struct obl_transaction *obl_begin_transaction(struct obl_session *s)
{
    struct obl_transaction *t;

    sem_wait(&s->lock);
    t = _allocate_transaction(s);
    sem_post(&s->lock);

    return t;
}

struct obl_transaction *obl_ensure_transaction(struct obl_session *s,
        int *created)
{
    struct obl_transaction *t;

    sem_wait(&s->lock);
    if (s->current_transaction != NULL) {
        *created = 0;
        t = s->current_transaction;
    } else {
        *created = 1;
        t = _allocate_transaction(s);
    }
    sem_post(&s->lock);

    return t;
}

void obl_mark_dirty(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_transaction *t;

    if (s == NULL || s->current_transaction == NULL ||
            o->logical_address == OBL_LOGICAL_UNASSIGNED)
        return ;

    sem_wait(&s->lock);
    t = s->current_transaction;
    obl_set_insert(t->write_set, o);
    sem_post(&s->lock);
}

int obl_commit_transaction(struct obl_transaction *t)
{
    struct obl_set_iterator *it;
    struct obl_object *current;
    struct obl_session *s = t->session;
    struct obl_database *d = s->database;

    sem_wait(&s->lock);
    sem_wait(&d->lock);

    it = obl_set_destroying_iter(t->write_set);
    while ( (current = obl_set_iternext(it)) != NULL ) {
        if (current->physical_address != OBL_PHYSICAL_UNASSIGNED) {
            _obl_write(current);
        }
    }
    obl_set_destroyiter(it);
    _deallocate_transaction(t);

    sem_post(&d->lock);
    sem_post(&s->lock);

    return 0;
}

void obl_abort_transaction(struct obl_transaction *t)
{
    obl_destroy_set(t->write_set, NULL);
    _deallocate_transaction(t);
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
