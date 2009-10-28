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
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    return _obl_resolve_stub(shape->storage.shape_storage->name);
}

struct obl_object *obl_shape_slotnames(struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotnames invoked with a non SHAPE object.");
        return obl_nil(shape->database);
    }

    return _obl_resolve_stub(shape->storage.shape_storage->slot_names);
}

obl_uint obl_shape_slotcount(struct obl_object *shape)
{
    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcount invoked with a non SHAPE object.");
        return OBL_SENTINEL;
    }

    return obl_fixed_size(obl_shape_slotnames(shape));
}

obl_uint obl_shape_slotnamed(struct obl_object *shape,
        struct obl_object *name)
{
    struct obl_object *slots;
    obl_uint i;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotnamed invoked with a non SHAPE object.");
        return OBL_SENTINEL;
    }

    slots = obl_shape_slotnames(shape);
    for (i = 0; i < obl_fixed_size(slots); i++) {
        if (obl_string_cmp(obl_fixed_at(slots, i), name) == 0) {
            return i;
        }
    }

    return OBL_SENTINEL;
}

obl_uint obl_shape_slotcnamed(struct obl_object *shape,
        const char *name)
{
    struct obl_object *temporary;
    obl_uint result;

    if (obl_storage_of(shape) != OBL_SHAPE) {
        obl_report_error(shape->database, OBL_WRONG_STORAGE,
                "obl_shape_slotcnamed invoked with a non SHAPE object.");
        return OBL_SENTINEL;
    }

    temporary = obl_create_cstring(shape->database, name, strlen(name));
    if (temporary == NULL) {
        return OBL_SENTINEL;
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

    return _obl_resolve_stub(shape->storage.shape_storage->current_shape);
}

enum obl_storage_type obl_shape_storagetype(struct obl_object *shape)
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

struct obl_object *obl_shape_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address base, int depth)
{
    struct obl_object *result;
    obl_logical_address addr;
    struct obl_object *name, *slot_names, *current_shape;
    obl_uint storage_format;

    addr = (obl_logical_address) readable_uint(source[base + 1]);
    name = obl_at_address_depth(shape->database, addr, depth - 1);

    addr = (obl_logical_address) readable_uint(source[base + 2]);
    slot_names = obl_at_address_depth(shape->database, addr, depth - 1);

    addr = (obl_logical_address) readable_uint(source[base + 3]);
    current_shape = obl_at_address_depth(shape->database, addr, depth - 1);

    storage_format = readable_uint(source[base + 4]);
    if (storage_format > OBL_STORAGE_TYPE_MAX) {
        obl_report_errorf(shape->database, OBL_WRONG_STORAGE,
                "Shape at physical address %ul has invalid storage format.",
                (unsigned long) base);
    }

    result = obl_create_shape(shape->database,
            name, slot_names, storage_format);
    result->storage.shape_storage->current_shape = current_shape;
    return result;
}

void obl_shape_write(struct obl_object *shape, obl_uint *dest)
{
    struct obl_object *name, *slot_names, *current_shape;

    name = shape->storage.shape_storage->name;
    slot_names = shape->storage.shape_storage->slot_names;
    current_shape = shape->storage.shape_storage->current_shape;

    dest[shape->physical_address + 1] = writable_uint(
            (obl_uint) name->logical_address);
    dest[shape->physical_address + 2] = writable_uint(
            (obl_uint) slot_names->logical_address);
    dest[shape->physical_address + 3] = writable_uint(
            (obl_uint) current_shape->logical_address);

    dest[shape->physical_address + 4] = writable_uint(
            (obl_uint) obl_shape_storagetype(shape));
}

void obl_shape_print(struct obl_object *shape, int depth, int indent)
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

obl_uint _obl_shape_children(struct obl_object *shape,
        struct obl_object **results, int *heaped)
{
    struct obl_shape_storage *storage;

    storage = shape->storage.shape_storage;
    *heaped = 1;
    results = (struct obl_object **) malloc(sizeof(struct obl_object*) * 3);
    results[0] = storage->name;
    results[1] = storage->slot_names;
    results[2] = storage->current_shape;

    return (obl_uint) 3;
}
