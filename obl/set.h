/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implements a scalable obl_object set with a red-black tree.
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

/**
 * Each obl_set has a key function which will be used to compute node keys from
 * obl_object instances.  The two most common, logical_address_keyfunction() and
 * heap_address_keyfunction(), are defined below.
 */
typedef obl_set_key (*obl_set_keyfunction)(struct obl_object *);

/**
 * The externally visible set structure.  Allocate them with obl_create_set.
 */
struct obl_set {

    /**
     * The root node of the red-black tree.  NULL if the set is empty.
     */
    struct obl_rb_node *root;

    /**
     * A function for extracting an obl_set_key from each obl_object that is
     * added.
     */
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
 * Add a new obl_object to the set.  If an object with the same key is
 * already present, it will be deallocated and this object will be inserted
 * in its place.
 *
 * \param set The set to add to.
 * \param o The object to add.
 */
void obl_set_insert(struct obl_set *set, struct obl_object *o);

/**
 * Return an obl_object that is currently mapped to a provided obl_set_key.
 *
 * \param set The set possibly containing the object.
 * \param key Key value to query.
 */
struct obl_object *obl_set_lookup(struct obl_set *set, obl_set_key key);

/**
 * Remove an obl_object from the set.
 *
 * \param set The set to remove from.
 * \param o The object to remove.  Note that this call will actually remove any
 *      object currently mapped to the same key as o, not necessarily o itself.
 */
void obl_set_remove(struct obl_set *set, struct obl_object *o);

/**
 * An obl_set_keyfunction that produces a map from assigned logical addresses
 * to obl_object instances.
 */
obl_set_key logical_address_keyfunction(struct obl_object *o);

/**
 * An obl_set_keyfunction that maps heap addresses to obl_object instances.
 * This is predominantly useful for newly created obl_objects that have not
 * been assigned logical addresses yet.
 */
obl_set_key heap_address_keyfunction(struct obl_object *o);

/**
 * Test an obl_set's internal structure for consistency.
 *
 * \param set The set to verify.
 * \return 1 if the set's structure is valid, 0 if it is not.  Error messages
 *      describing the kind(s) of violation discovered will print to stderr.
 */
int obl_set_verify(struct obl_set *set);

/**
 * Pretty-print an obl_set to standard out.  Um, only call this with small sets
 * unless you don't like responsive terminals.
 *
 * \param set The set to print.
 */
void obl_set_print(struct obl_set *set);

#endif /* SET_H */