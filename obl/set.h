/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#ifndef SET_H
#define SET_H

#include <stdint.h>

#include "storage/object.h"
#include "platform.h"

/* defined in set.c */
struct obl_rb_node;

/**
 * obl_set_key must be able to store both obl_address and uintptr_t values, so
 * use whichever has greater width.
 */
#if UINTPTR_MAX > OBL_ADDRESS_MAX
typedef uintptr_t obl_set_key;
#else
typedef obl_address obl_set_key;
#endif

typedef obl_set_key (*obl_set_keyfunction)(struct obl_object *);

struct obl_set {

    struct obl_rb_node *root;

    obl_set_keyfunction keyfunction;

};

/**
 * Create an obl_set that uses the provided function to generate item keys.
 *
 * \param keyfunction Pointer to a function that derives obl_set_key values
 *      from obl_object instances.
 * \return A newly allocated obl_set, or NULL if the allocation failed.
 */
struct obl_set *obl_create_set(obl_set_keyfunction keyfunction);

/**
 * Test an obl_set's internal structure for consistency.
 *
 * \param set The set to verify.
 * \return 1 if the set's structure is valid, 0 if it is not.  Error messages
 *      describing the kind(s) of violation discovered will print to stderr.
 */
int obl_set_verify(struct obl_set *set);

void obl_set_print(struct obl_set *set);

void obl_set_insert(struct obl_set *set, struct obl_object *o);

struct obl_object *obl_set_lookup(struct obl_set *set, obl_set_key key);

void obl_set_remove(struct obl_set *set, struct obl_object *o);

obl_set_key logical_address_keyfunction(struct obl_object *o);

obl_set_key heap_address_keyfunction(struct obl_object *o);

#endif /* SET_H */
