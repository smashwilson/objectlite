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

    return slotted->storage.slotted_storage->slots[index];
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
