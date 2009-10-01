/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#include "database.h"

#include "log.h"
#include "constants.h"
#include "object.h"

#include <stdio.h>

/* Internal function prototypes. */

static inline int _is_fixed_address(const obl_logical_address addr);

static int _initialize_fixed_objects(obl_database *database);

/* External functions definitions. */

obl_database *obl_create_database(const char *filename)
{
    obl_database *database;
    obl_cache *cache;

    database = (obl_database*) malloc(sizeof(obl_database));
    if (database == NULL) {
        return NULL;
    }

    database->filename = filename;
    obl_clear_error(database);

    cache = obl_create_cache(DEFAULT_CACHE_BUCKETS, DEFAULT_CACHE_SIZE);
    if (cache == NULL) {
        obl_report_error(database, OUT_OF_MEMORY, "Unable to allocate cache.");
        free(database);
        return NULL;
    }
    database->cache = cache;

    if (_initialize_fixed_objects(database)) {
        obl_report_error(database, OUT_OF_MEMORY,
                "Unable to allocate fixed space.");
        obl_destroy_cache(database->cache);
        free(database);
        return NULL;
    }

    return database;
}

obl_object *obl_at_address(obl_database *database,
        const obl_logical_address address)
{
    /* Check for fixed addresses first. */
    if (_is_fixed_address(address)) {
        return database->fixed[address];
    }

    return NULL;
}

void obl_destroy_database(obl_database *database)
{
    int i;
    obl_object *o;

    if (database->cache != NULL) {
        obl_destroy_cache(database->cache);
    }

    for (i = 0; i < OBL_FIXED_ADDR_MAX; i++) {
        o = database->fixed[i];
        if (o->shape == NULL) {
            obl_destroy_cshape(o);
        } else {
            obl_destroy_object(database->fixed[i]);
        }
    }

    free(database);
}

int obl_database_ok(const obl_database *database)
{
    return database->last_error.code == OK;
}

void obl_clear_error(obl_database *database)
{
    database->last_error.message = NULL;
    database->last_error.code = OK;
}

void obl_report_error(obl_database *database, error_code code, char *message)
{
    OBL_ERROR(database, message);

    if (database == NULL) {
        fprintf(stderr, "Unable to report error \"%s\":\n", message);
        fprintf(stderr, "No database structure available to report it in.\n");
        return;
    }

    database->last_error.message = message;
    database->last_error.code = code;
}

/* Internal function implementations. */

static inline int _is_fixed_address(const obl_logical_address addr)
{
    return addr < OBL_FIXED_ADDR_MAX;
}

static int _initialize_fixed_objects(obl_database *database)
{
    int i;
    char *no_slots[0];

    database->fixed = (obl_object **) malloc(sizeof(obl_object*)
            * OBL_FIXED_ADDR_MAX);
    if (database->fixed == NULL) {
        return 1;
    }

    database->fixed[OBL_INTEGER_SHAPE_ADDR] = obl_create_cshape(database,
            "Integer", 0, no_slots, OBL_INTEGER);
    database->fixed[OBL_FLOAT_SHAPE_ADDR] = obl_create_cshape(database,
            "Float", 0, no_slots, OBL_FLOAT);
    database->fixed[OBL_DOUBLE_SHAPE_ADDR] = obl_create_cshape(database,
            "Double", 0, no_slots, OBL_DOUBLE);

    database->fixed[OBL_CHAR_SHAPE_ADDR] = obl_create_cshape(database,
            "Character", 0, no_slots, OBL_CHAR);
    database->fixed[OBL_STRING_SHAPE_ADDR] = obl_create_cshape(database,
            "String", 0, no_slots, OBL_STRING);

    database->fixed[OBL_FIXED_SHAPE_ADDR] = obl_create_cshape(database,
            "FixedCollection", 0, no_slots, OBL_FIXED);
    database->fixed[OBL_CHUNK_SHAPE_ADDR] = obl_create_cshape(database,
            "OblChunk", 0, no_slots, OBL_CHUNK);

    database->fixed[OBL_NIL_SHAPE_ADDR] = obl_create_cshape(database,
            "Undefined", 0, no_slots, OBL_NIL);
    database->fixed[OBL_BOOLEAN_SHAPE_ADDR] = obl_create_cshape(database,
            "Boolean", 0, no_slots, OBL_BOOLEAN);

    database->fixed[OBL_NIL_ADDR] = _obl_create_nil(database);
    database->fixed[OBL_TRUE_ADDR] = _obl_create_bool(database, 1);
    database->fixed[OBL_FALSE_ADDR] = _obl_create_bool(database, 0);

    /* Set logical addresses of these objects. */
    for (i = 0; i < OBL_FIXED_ADDR_MAX; i++) {
        database->fixed[i]->logical_address = (obl_logical_address) i;
    }

    return 0;
}
