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
    if (database->cache != NULL) {
        obl_destroy_cache(database->cache);
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
    database->fixed = (obl_object **) malloc(sizeof(obl_object*)
            * OBL_FIXED_ADDR_MAX);
    if (database->fixed == NULL) {
        return 1;
    }

    database->fixed[OBL_INTEGER_SHAPE_ADDR] = NULL;
    database->fixed[OBL_FLOAT_SHAPE_ADDR] = NULL;
    database->fixed[OBL_DOUBLE_SHAPE_ADDR] = NULL;

    database->fixed[OBL_CHAR_SHAPE_ADDR] = NULL;
    database->fixed[OBL_STRING_SHAPE_ADDR] = NULL;

    database->fixed[OBL_FIXED_SHAPE_ADDR] = NULL;
    database->fixed[OBL_CHUNK_SHAPE_ADDR] = NULL;

    database->fixed[OBL_NIL_SHAPE_ADDR] = NULL;
    database->fixed[OBL_BOOLEAN_SHAPE_ADDR] = NULL;

    database->fixed[OBL_NIL_ADDR] = NULL;
    database->fixed[OBL_TRUE_ADDR] = NULL;
    database->fixed[OBL_FALSE_ADDR] = NULL;

    return 0;
}
