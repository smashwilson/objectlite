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
#include <stdarg.h>

/* Internal function prototypes. */

static inline int _is_fixed_address(const obl_logical_address addr);

static int _initialize_fixed_objects(obl_database *database);

/* Error codes: one for each error_code in error.h. */

static char *error_messages[] = {
        NULL,
        "Unable to allocate an object", "Unable to read file",
        "Unable to open file", "Error during Unicode conversion",
        "Incorrect object storage type", "Bad argument length"
};

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
    database->last_error.message = NULL;
    obl_clear_error(database);

    cache = obl_create_cache(DEFAULT_CACHE_BUCKETS, DEFAULT_CACHE_SIZE);
    if (cache == NULL) {
        obl_report_error(database, OBL_OUT_OF_MEMORY, "Unable to allocate cache.");
        free(database);
        return NULL;
    }
    database->cache = cache;

    if (_initialize_fixed_objects(database)) {
        obl_report_error(database, OBL_OUT_OF_MEMORY,
                "Unable to allocate fixed space.");
        obl_destroy_cache(database->cache);
        free(database);
        return NULL;
    }

    return database;
}

struct obl_object *obl_at_address(obl_database *database,
        const obl_logical_address address)
{
    /* Check for fixed addresses first. */
    if (_is_fixed_address(address)) {
        return database->fixed[address];
    }

    return NULL;
}

struct obl_object *obl_nil(obl_database *database)
{
    return obl_at_address(database, OBL_NIL_ADDR);
}

struct obl_object *obl_true(obl_database *database)
{
    return obl_at_address(database, OBL_TRUE_ADDR);
}

struct obl_object *obl_false(obl_database *database)
{
    return obl_at_address(database, OBL_FALSE_ADDR);
}

void obl_destroy_database(obl_database *database)
{
    int i;
    struct obl_object *o;

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

    if (database->last_error.message != NULL) {
        free(database->last_error.message);
    }

    free(database);
}

int obl_database_ok(const obl_database *database)
{
    return database->last_error.code == OBL_OK;
}

void obl_clear_error(obl_database *database)
{
    if (database->last_error.message != NULL) {
        free(database->last_error.message);
    }
    database->last_error.message = NULL;
    database->last_error.code = OBL_OK;
}

void obl_report_error(obl_database *database, error_code code, const char *message)
{
    char *buffer;
    size_t message_size;
    OBL_ERROR(database, message);

    if (database == NULL) {
        fprintf(stderr, "Unable to report error \"%s\":\n", message);
        fprintf(stderr, "No database structure available to report it in.\n");
        return;
    }

    if (message != NULL) {
        message_size = strlen(message);
        buffer = (char*) malloc(message_size);
        memcpy(buffer, message, message_size);

        database->last_error.message = buffer;
    } else {
        database->last_error.message = error_messages[code];
    }
    database->last_error.code = code;
}

void obl_report_errorf(obl_database *database, error_code code,
        const char *format, ...)
{
    va_list args;
    size_t required_size;
    char *buffer;

    /* Include the terminating NULL byte. */
    va_start(args, format);
    required_size = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    va_start(args, format);
    buffer = (char*) malloc(required_size);
    vsnprintf(buffer, required_size, format, args);
    va_end(args);

    obl_report_error(database, code, buffer);

    free(buffer);
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

    database->fixed = (struct obl_object **) malloc(sizeof(struct obl_object*)
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
