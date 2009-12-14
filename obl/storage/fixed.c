/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/fixed.h"

#include "storage/object.h"
#include "database.h"
#include "session.h"

#include <stdio.h>
#include <stdlib.h>

struct obl_object *obl_create_fixed(obl_uint length)
{
    struct obl_object *result;
    struct obl_fixed_storage *storage;
    obl_uint i;

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }

    storage = malloc(sizeof(struct obl_fixed_storage));
    if (storage == NULL) {
        obl_report_error(NULL, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.fixed_storage = storage;
    result->shape = _obl_at_fixed_address(OBL_FIXED_SHAPE_ADDR);

    storage->length = length;
    storage->contents = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * length);
    if (storage->contents == NULL) {
        obl_report_error(NULL, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        free(storage);
        return NULL;
    }

    for (i = 0; i < length; i++) {
        storage->contents[i] = obl_nil();
    }

    return result;
}

obl_uint obl_fixed_size(struct obl_object *fixed)
{
    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(obl_database_of(fixed), OBL_WRONG_STORAGE,
                "obl_fixed_size requires an object with FIXED storage.");
        return 0;
    }

    return fixed->storage.fixed_storage->length;
}

struct obl_object *obl_fixed_at(struct obl_object *fixed, obl_uint index)
{
    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(obl_database_of(fixed), OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return obl_nil();
    }

    if (index >= obl_fixed_size(fixed)) {
        obl_report_errorf(obl_database_of(fixed), OBL_INVALID_INDEX,
                "obl_fixed_at called with an invalid index (%d, valid 0..%d)",
                index,
                obl_fixed_size(fixed) - 1);
        return obl_nil();
    }

    return _obl_resolve_stub(fixed->storage.fixed_storage->contents[index]);
}

void obl_fixed_at_put(struct obl_object *fixed, const obl_uint index,
        struct obl_object *value)
{
    struct obl_session *s = fixed->session;
    struct obl_transaction *t;
    int created = 0;

    if (obl_storage_of(fixed) != OBL_FIXED) {
        obl_report_error(obl_database_of(fixed), OBL_WRONG_STORAGE,
                        "obl_fixed_at requires an object with FIXED storage.");
        return ;
    }

    if (index >= obl_fixed_size(fixed)) {
        obl_report_errorf(obl_database_of(fixed), OBL_INVALID_INDEX,
                "obl_fixed_at_put called with an invalid index (%d, valid 0..%d)",
                index,
                obl_fixed_size(fixed) - 1);
        return ;
    }

    t = obl_ensure_transaction(s, &created);

    obl_mark_dirty(fixed);
    fixed->storage.fixed_storage->contents[index] = value;

    if (created) obl_commit_transaction(t);
}

struct obl_object *obl_fixed_read(struct obl_session *session,
        struct obl_object *shape, obl_uint *source,
        obl_physical_address base, int depth)
{
    obl_uint length;
    obl_uint i;
    struct obl_object *o;
    obl_logical_address addr;
    struct obl_object *linked;

    length = readable_uint(source[base + 1]);
    o = obl_create_fixed(length);

    for (i = 0; i < length; i++) {
        addr = readable_logical(source[base + 2 + i]);
        linked = _obl_at_address_depth(session, addr, depth - 1, 0);
        obl_fixed_at_put(o, i, linked);
    }

    return o;
}

void obl_fixed_write(struct obl_object *fixed, obl_uint *dest)
{
    obl_uint length;
    obl_uint i;
    struct obl_object *linked;

    length = obl_fixed_size(fixed);
    dest[fixed->physical_address + 1] = writable_uint(length);

    for (i = 0; i < length; i++) {
        /* Avoid unnecessarily resolving any stubs. */
        linked = fixed->storage.fixed_storage->contents[i];

        dest[fixed->physical_address + 2 + i] = writable_uint(
                (obl_uint) linked->logical_address);
    }
}

void obl_fixed_print(struct obl_object *fixed, int depth, int indent)
{
    int ind, i;

    for (ind = 0; ind < indent; ind++) { putchar(' '); }
    if (depth == 0) {
        printf("<fixed collection: %d elements>\n",
                obl_fixed_size(fixed));
        return ;
    }
    puts("Fixed Collection");

    for (i = 0; i < obl_fixed_size(fixed); i++) {
        obl_print_object(obl_fixed_at(fixed, i), depth - 1, indent + 2);
        printf("\n");
    }
}

struct obl_object_list *_obl_fixed_children(struct obl_object *fixed)
{
    struct obl_fixed_storage *storage = fixed->storage.fixed_storage;
    struct obl_object_list *results = NULL;
    obl_uint fixed_size, i;

    obl_object_list_append(&results, fixed->shape);

    fixed_size = storage->length;
    for (i = 0; i < fixed_size; i++) {
        obl_object_list_append(&results, storage->contents[i]);
    }

    return results;
}

void _obl_fixed_deallocate(struct obl_object *fixed)
{
    free(fixed->storage.fixed_storage->contents);
    free(fixed->storage.fixed_storage);
}
