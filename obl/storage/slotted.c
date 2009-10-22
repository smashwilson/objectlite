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

struct obl_object *obl_create_slotted(struct obl_object *shape)
{
    struct obl_object *result;
    struct obl_slotted_storage *storage;
    obl_uint slot_count;
    struct obl_object **slots;
    obl_uint i;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_create_slotted requires a SHAPE object.");
        return NULL;
    }

    result = _obl_allocate_object(shape->database);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_slotted_storage*)
            malloc(sizeof(struct obl_slotted_storage));
    if (storage == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.slotted_storage = storage;
    result->shape = shape;

    slot_count = obl_shape_slotcount(shape);
    slots = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * slot_count);
    if (slots == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        free(storage);
        return NULL;
    }
    storage->slots = slots;

    for (i = 0; i < slot_count; i++) {
        slots[i] = obl_nil(shape->database);
    }

    return result;
}

struct obl_object *obl_slotted_at(const struct obl_object *slotted,
        const obl_uint index)
{
    if (obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(slotted->database, OBL_WRONG_STORAGE,
                "obl_slotted_at requires a SLOTTED object.");
        return obl_nil(slotted->database);
    }

    return _obl_resolve_stub(slotted->storage.slotted_storage->slots[index]);
}

struct obl_object *obl_slotted_atnamed(const struct obl_object *slotted,
        const struct obl_object *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotnamed(slotted->shape, slotname));
}

struct obl_object *obl_slotted_atcnamed(const struct obl_object *slotted,
        const char *slotname)
{
    return obl_slotted_at(slotted,
            obl_shape_slotcnamed(slotted->shape, slotname));
}

void obl_slotted_at_put(struct obl_object *slotted,
        const obl_uint index, struct obl_object *value)
{
    if (obl_storage_of(slotted) != OBL_SLOTTED) {
        obl_report_error(slotted->database, OBL_WRONG_STORAGE,
                "obl_slotted_at_put requires a SLOTTED object.");
        return ;
    }

    slotted->storage.slotted_storage->slots[index] = value;
}

void obl_slotted_atnamed_put(struct obl_object *slotted,
        const struct obl_object *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotnamed(slotted->shape, slotname), value);
}

void obl_slotted_atcnamed_put(struct obl_object *slotted,
        const char *slotname, struct obl_object *value)
{
    obl_slotted_at_put(slotted, obl_shape_slotcnamed(slotted->shape, slotname), value);
}

void obl_print_slotted(struct obl_object *slotted, int depth, int indent)
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
