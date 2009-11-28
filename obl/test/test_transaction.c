/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file test_transaction.c
 */

#include "transaction.h"

#include "storage/integer.h"
#include "database.h"
#include "session.h"
#include "unitutilities.h"

#include "CUnit/Basic.h"

static const char *filename = "transaction.obl";

void test_ensure_transaction(void)
{
    int created = 0;
    struct obl_database *d = obl_create_database(filename);
    struct obl_session *s = obl_create_session(d);
    struct obl_transaction *t, *same;

    CU_ASSERT(s->current_transaction == NULL);

    t = obl_ensure_transaction(s, &created);
    CU_ASSERT(t != NULL);
    CU_ASSERT(created == 1);
    CU_ASSERT(s->current_transaction == t);
    obl_abort_transaction(t);

    CU_ASSERT(s->current_transaction == NULL);

    t = obl_begin_transaction(s);
    CU_ASSERT(s->current_transaction == t);
    created = 0;
    same = obl_ensure_transaction(s, &created);
    CU_ASSERT(created == 0);
    CU_ASSERT(same == t);
    obl_abort_transaction(t);

    obl_destroy_session(s);
    obl_destroy_database(d);
}

/*
 * Collect the unit tests defined here into a CUnit test suite.  Return the
 * initialized suite on success, or NULL on failure.  Invoked by unittests.c.
 */
CU_pSuite initialize_transaction_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("transaction", NULL, NULL);
    if (pSuite == NULL) {
        return NULL;
    }

    ADD_TEST(test_ensure_transaction);

    return pSuite;
}
