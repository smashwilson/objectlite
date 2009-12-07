/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/slotted.h"

#include "storage/object.h"
#include "database.h"
#include "session.h"

#include <stdlib.h>
#include <stdio.h>

struct obl_object *obl_create_slotted(struct obl_object *shape)
{
    struct obl_object *result;
    struct obl_slotted_storage *storage;
    obl_uint slot_count;
    struct obl_object **slots;
    obl_uint i;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(obl_database_of(shape), OBL_WRONG_STORAGE,
                "obl_create_slotted requires a SHAPE object.");
        return NULL;
    }

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }

    storage = malloc(sizeof(struct obl_slotted_storage));
    if (storage == NULL) {
        obl_report_error(obl_database_of(shape), OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.slotted_storage = storage;
    result->shape = shape;

    slot_count = obl_shape_slotcount(shape);
    slots = malloc(sizeof(struct obl_object*) * slot_count);
    if (slots == NULL) {
        obl_report_error(obl_database_of(shape), OBL_OUT_OF_MEMORY, NULL);
        free(result);
        free(storage);
        return NULL;
    }
    storage->slots = slots;

    for (i = 0; i < slot_count; i++) {
        slots[i] = obl_nil();
    }

    return result;
}

struct obl_object *obl_slotted_at(struct obl_object *slotted, obl_uint index)
{
    obl_uint maximum;

    if (obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(obl_database_of(slotted), OBL_WRONG_STORAGE,
                "obl_slotted_at requires a SLOTTED object.");
        return obl_nil();
    }

    maximum = obl_shape_slotcount(obl_object_shape(slotted));
    if (index >= maximum) {
        obl_report_errorf(obl_database_of(slotted), OBL_INVALID_INDEX,
                "obl_slotted_at called with an invalid index (%d, valid 0..%d)",
                index, maximum - 1);
        return obl_nil();
    }

    return _obl_resolve_stub(slotted->storage.slotted_storage->slots[index]);
}

struct obl_object *obl_slotted_atnamed(struct obl_object *slotted,
        struct obl_object *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotnamed(slotted->shape, slotname));
}

struct obl_object *obl_slotted_atcnamed(struct obl_object *slotted,
        const char *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotcnamed(slotted->shape, slotname));
}

void obl_slotted_at_put(struct obl_object *slotted,
        obl_uint index, struct obl_object *value)
{
    obl_uint maximum;

    if (obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(obl_database_of(slotted), OBL_WRONG_STORAGE,
                "obl_slotted_at_put requires a SLOTTED object.");
        return ;
    }

    maximum = obl_shape_slotcount(obl_object_shape(slotted));
    if (index >= maximum) {
        obl_report_errorf(obl_database_of(slotted), OBL_INVALID_INDEX,
                "obl_slotted_at_put called with an invalid index (%d, valid 0..%d)",
                index, maximum - 1);
        return ;
    }

    slotted->storage.slotted_storage->slots[index] = value;
}

void obl_slotted_atnamed_put(struct obl_object *slotted,
        struct obl_object *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotnamed(slotted->shape, slotname), value);
}

void obl_slotted_atcnamed_put(struct obl_object *slotted,
        const char *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotcnamed(slotted->shape, slotname), value);
}

struct obl_object *obl_slotted_read(struct obl_session *session,
        struct obl_object *shape, obl_uint *source,
        obl_physical_address base, int depth)
{
    struct obl_object *result;
    obl_uint slot_count;
    obl_uint i;
    obl_logical_address addr;
    struct obl_object *linked;

    result = obl_create_slotted(shape);

    slot_count = obl_shape_slotcount(shape);
    for (i = 0; i < slot_count; i++) {
        addr = readable_logical(source[base + 1 + i]);
        linked = _obl_at_address_depth(session, addr, depth - 1, 0);
        obl_slotted_at_put(result, i, linked);
    }

    return result;
}

void obl_slotted_write(struct obl_object *slotted, obl_uint *dest)
{
    obl_uint slot_count;
    obl_uint i;
    struct obl_object *linked;

    slot_count = obl_shape_slotcount(slotted->shape);
    for (i = 0; i < slot_count; i++) {
        /* Avoid unnecessary stub resolution. */
        linked = slotted->storage.slotted_storage->slots[i];

        dest[slotted->physical_address + 1 + i] = writable_uint(
                (obl_uint) linked->logical_address);
    }
}

void obl_slotted_print(struct obl_object *slotted, int depth, int indent)
{
    int in, slot_i;
    struct obl_object *shape, *slotnames, *slotname, *slot;

    shape = obl_object_shape(slotted);
    for (in = 0; in < indent; in++) { putchar(' '); }
    if (depth == 0) {
        struct obl_object *name;

        name = obl_shape_name(shape);
        printf("<slotted:");
        obl_print_object(name, 0, 0);
        printf(">");
        return ;
    }
    puts("Slotted Object");

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("Shape: ");
    obl_print_object(shape, 0, 0);
    printf("\n");

    slotnames = obl_shape_slotnames(shape);
    for (slot_i = 0; slot_i < obl_shape_slotcount(shape); slot_i++) {
        slotname = obl_fixed_at(slotnames, slot_i);
        slot = obl_slotted_at(slotted, slot_i);

        for (in = 0; in < indent; in++) { putchar(' '); }
        obl_print_object(slotname, 0, 0);
        printf(":\n");
        obl_print_object(slot, depth - 1, indent + 2);
        printf("\n");
    }
}

obl_uint _obl_slotted_children(struct obl_object *slotted,
        struct obl_object **results, int *heaped)
{
    results = slotted->storage.slotted_storage->slots;
    return obl_shape_slotcount(obl_object_shape(slotted));
}

void _obl_slotted_deallocate(struct obl_object *slotted)
{
    free(slotted->storage.slotted_storage->slots);
    free(slotted->storage.slotted_storage);
}
