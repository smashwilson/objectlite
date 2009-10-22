/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/shape.h"

#include "storage/object.h"
#include "database.h"

struct obl_object *obl_create_shape(struct obl_database *d,
        struct obl_object *name, struct obl_object *slot_names,
        enum obl_storage_type type)
{
    struct obl_object *result;
    struct obl_shape_storage *storage;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_shape_storage*)
            malloc(sizeof(struct obl_shape_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->shape = obl_nil(d);

    storage->name = name;
    storage->slot_names = slot_names;
    storage->current_shape = obl_nil(d);
    storage->storage_format = (obl_uint) type;
    result->storage.shape_storage = storage;

    return result;
}

struct obl_object *obl_create_cshape(struct obl_database *d,
        char *name, size_t slot_count, char **slot_names,
        enum obl_storage_type type)
{
    struct obl_object *name_ob, *slots_ob, *slot_name_ob;
    struct obl_object *result;
    int i, j;

    name_ob = obl_create_cstring(d, name, strlen(name));
    if (name_ob == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    slots_ob = obl_create_fixed(d, slot_count);
    if (slots_ob == NULL) {
        obl_destroy_object(name_ob);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    for (i = 0; i < slot_count; i++) {
        slot_name_ob = obl_create_cstring(d,
                slot_names[i], strlen(slot_names[i]));
        if (slot_name_ob == NULL) {
            for (j = 0; j < i; j++) {
                obl_destroy_object(obl_fixed_at(slots_ob, j));
            }
            obl_destroy_object(slots_ob);
            obl_destroy_object(name_ob);
            obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
            return NULL;
        }
        obl_fixed_at_put(slots_ob, i, slot_name_ob);
    }

    result = obl_create_shape(d, name_ob, slots_ob, type);
    if (result == NULL) {
        for (j = 0; j < slot_count; j++) {
            obl_destroy_object(obl_fixed_at(slots_ob, j));
        }
        obl_destroy_object(slots_ob);
        obl_destroy_object(name_ob);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    return result;
}

struct obl_object *obl_shape_name(struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_name invoked with a non SHAPE object.");
        return obl_nil(shape->database);
    }

    return shape->storage.shape_storage->name;
}

struct obl_object *obl_shape_slotnames(struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotnames invoked with a non SHAPE object.");
        return obl_nil(shape->database);
    }

    return shape->storage.shape_storage->slot_names;
}

obl_uint obl_shape_slotcount(const struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcount invoked with a non SHAPE object.");
        return 0;
    }

    return obl_fixed_size(shape->storage.shape_storage->slot_names);
}

obl_uint obl_shape_slotnamed(const struct obl_object *shape,
        const struct obl_object *name)
{
    struct obl_object *slots;
    obl_uint i;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotnamed invoked with a non SHAPE object.");
        return 0;
    }

    slots = shape->storage.shape_storage->slot_names;
    for (i = 0; i < obl_fixed_size(slots); i++) {
        if (obl_string_cmp(obl_fixed_at(slots, i), name) == 0) {
            return i;
        }
    }

    return -1;
}

obl_uint obl_shape_slotcnamed(const struct obl_object *shape,
        const char *name)
{
    struct obl_object *temporary;
    obl_uint result;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcnamed invoked with a non SHAPE object.");
        return 0;
    }

    temporary = obl_create_cstring(shape->database, name, strlen(name));
    if (temporary == NULL) {
        return -1;
    }
    result = obl_shape_slotnamed(shape, temporary);
    obl_destroy_object(temporary);

    return result;
}

struct obl_object *obl_shape_currentshape(struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_currentshape invoked with a non SHAPE object.");
        return obl_nil(shape->database);
    }

    return shape->storage.shape_storage->current_shape;
}

enum obl_storage_type obl_shape_storagetype(const struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_storagetype invoked with a non SHAPE object.");
        return 0;
    }

    return (enum obl_storage_type)
            shape->storage.shape_storage->storage_format;
}

/* Free exactly those structures created by obl_create_shape. */
void obl_destroy_cshape(struct obl_object *shape)
{
    struct obl_shape_storage *storage;
    int slot_count, i;

    storage = shape->storage.shape_storage;
    obl_destroy_object(storage->name);

    slot_count = obl_fixed_size(storage->slot_names);
    for (i = 0; i < slot_count; i++) {
        obl_destroy_object(obl_fixed_at(storage->slot_names, i));
    }
    obl_destroy_object(storage->slot_names);
}

void obl_print_shape(struct obl_object *shape, int depth, int indent)
{
    int in;
    struct obl_object *name, *slots, *current_shape;

    name = obl_shape_name(shape);
    for (in = 0; in < indent; in++) { putchar(' '); }
    if (depth == 0) {
        printf("<shape:");
        obl_print_object(name, 0, 0);
        printf(">");
        return ;
    }

    puts("Shape");

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("Name: ");
    obl_print_object(name, 0, 0);
    printf("\n");

    slots = obl_shape_slotnames(shape);
    for (in = 0; in < indent; in++) { putchar(' '); }
    puts("Slots:");
    obl_print_object(slots, depth - 1, indent + 2);

    current_shape = obl_shape_currentshape(shape);
    for (in = 0; in < indent; in++) { putchar(' '); }
    puts("Current Shape:");
    obl_print_object(current_shape, depth - 1, indent + 2);
}
