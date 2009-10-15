/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#include "database.h"

#include "cache.h"
#include "constants.h"
#include "log.h"
#include "object.h"

#include <stdio.h>
#include <stdarg.h>

/* Internal function prototypes. */

static inline int _is_fixed_address(const obl_logical_address addr);

static int _initialize_fixed_objects(struct obl_database *database);

static inline int _index_for_fixed(obl_logical_address addr);

/* Error codes: one for each error_code in error.h. */

static char *error_messages[] = {
        "EVERYTHING IS FINE",
        "Unable to allocate an object", "Unable to read file",
        "Unable to open file", "Error during Unicode conversion",
        "Incorrect object storage type", "Bad argument length"
};

/* External functions definitions. */

struct obl_database *obl_create_database(const char *filename)
{
    struct obl_database *database;
    struct obl_cache *cache;

    database = (struct obl_database*) malloc(sizeof(struct obl_database));
    if (database == NULL) {
        return NULL;
    }

    database->filename = filename;
    database->log_config.filename = NULL;
    database->log_config.level = L_DEBUG;
    database->last_error.code = OBL_OK;
    database->last_error.message = NULL;
    database->default_stub_depth = DEFAULT_STUB_DEPTH;

    /* Initialize +root+ to OBL_UNASSIGNED until opened. */
    database->root.address_map = OBL_PHYSICAL_UNASSIGNED;
    database->root.allocator = OBL_PHYSICAL_UNASSIGNED;
    database->root.name_map = OBL_PHYSICAL_UNASSIGNED;
    database->root.shape_map = OBL_PHYSICAL_UNASSIGNED;
    database->root.dirty = 0;

    cache = obl_create_cache(DEFAULT_CACHE_BUCKETS, DEFAULT_CACHE_SIZE);
    if (cache == NULL) {
        obl_report_error(database, OBL_OUT_OF_MEMORY,
                "Unable to allocate cache.");
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

    database->content = NULL;

    return database;
}

struct obl_object *obl_at_address(struct obl_database *database,
        const obl_logical_address address)
{
    return obl_at_address_depth(database, address,
            database->default_stub_depth);
}

struct obl_object *obl_at_address_depth(struct obl_database *database,
        const obl_logical_address address, int depth)
{
    struct obl_object *o;

    /* Check for fixed addresses first. */
    if (_is_fixed_address(address)) {
        return database->fixed[_index_for_fixed(address)];
    }

    /* Check for a cache hit. */
    o = obl_cache_get(database->cache, address);
    if (o != NULL) {
        return o;
    }

    return NULL;
}

struct obl_object *obl_nil(struct obl_database *database)
{
    return obl_at_address(database, OBL_NIL_ADDR);
}

struct obl_object *obl_true(struct obl_database *database)
{
    return obl_at_address(database, OBL_TRUE_ADDR);
}

struct obl_object *obl_false(struct obl_database *database)
{
    return obl_at_address(database, OBL_FALSE_ADDR);
}

void obl_destroy_database(struct obl_database *database)
{
    int i;
    struct obl_object *o;

    if (database->cache != NULL) {
        obl_destroy_cache(database->cache);
    }

    for (i = 0; i < OBL_FIXED_SIZE; i++) {
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

int obl_database_ok(const struct obl_database *database)
{
    return database->last_error.code == OBL_OK;
}

void obl_clear_error(struct obl_database *database)
{
    if (database->last_error.message != NULL) {
        free(database->last_error.message);
    }
    database->last_error.message = NULL;
    database->last_error.code = OBL_OK;
}

void obl_report_error(struct obl_database *database,
        error_code code, const char *message)
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
        message_size = strlen(message) + 1;
        buffer = (char*) malloc(message_size);
        memcpy(buffer, message, message_size);

        database->last_error.message = buffer;
    } else {
        database->last_error.message = error_messages[code];
    }
    database->last_error.code = code;
}

void obl_report_errorf(struct obl_database *database, error_code code,
        const char *format, ...)
{
    va_list args;
    size_t required_size;
    char *buffer;

    /* Include the terminating NULL byte in required_size. */
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
    return addr >= OBL_FIXED_ADDR_MIN;
}

static int _initialize_fixed_objects(struct obl_database *database)
{
    int i;
    obl_logical_address addr;
    char *no_slots[0];
    struct obl_object *fixed_shape, *string_shape, *undefined_shape;
    struct obl_object *nil;

    database->fixed = (struct obl_object **)
            malloc(sizeof(struct obl_object*) * OBL_FIXED_SIZE);
    if (database->fixed == NULL) {
        return 1;
    }

    for (i = 0; i < OBL_FIXED_SIZE; i++) {
        database->fixed[i] = NULL;
    }

    /*
     * The FixedCollection, String, Undefined shapes and Nil object are used
     * inside of shape objects (including their own).  Create these objects
     * first and manually correct the structures of their shape members.
     */
    fixed_shape = obl_create_cshape(database, "FixedCollection",
            0, no_slots, OBL_FIXED);
    string_shape = obl_create_cshape(database, "String",
            0, no_slots, OBL_STRING);
    undefined_shape = obl_create_cshape(database, "Undefined",
            0, no_slots, OBL_NIL);
    nil = _obl_create_nil(database);

    fixed_shape->shape = nil;
    fixed_shape->storage.shape_storage->current_shape = nil;
    fixed_shape->storage.shape_storage->name->shape = string_shape;
    fixed_shape->storage.shape_storage->slot_names->shape = fixed_shape;
    string_shape->shape = nil;
    string_shape->storage.shape_storage->current_shape = nil;
    string_shape->storage.shape_storage->name->shape = string_shape;
    string_shape->storage.shape_storage->slot_names->shape = fixed_shape;
    undefined_shape->shape = nil;
    undefined_shape->storage.shape_storage->current_shape = nil;
    undefined_shape->storage.shape_storage->name->shape = string_shape;
    undefined_shape->storage.shape_storage->slot_names->shape = fixed_shape;
    nil->shape = undefined_shape;

    database->fixed[_index_for_fixed(OBL_FIXED_SHAPE_ADDR)] = fixed_shape;
    database->fixed[_index_for_fixed(OBL_STRING_SHAPE_ADDR)] = string_shape;
    database->fixed[_index_for_fixed(OBL_NIL_SHAPE_ADDR)] = undefined_shape;
    database->fixed[_index_for_fixed(OBL_NIL_ADDR)] = nil;

    /*
     * Allocate the rest of the fixed-space shape objects.
     */
    database->fixed[_index_for_fixed(OBL_INTEGER_SHAPE_ADDR)] =
            obl_create_cshape(database, "Integer", 0, no_slots, OBL_INTEGER);
    database->fixed[_index_for_fixed(OBL_FLOAT_SHAPE_ADDR)] =
            obl_create_cshape(database, "Float", 0, no_slots, OBL_FLOAT);
    database->fixed[_index_for_fixed(OBL_DOUBLE_SHAPE_ADDR)] =
            obl_create_cshape(database, "Double", 0, no_slots, OBL_DOUBLE);
    database->fixed[_index_for_fixed(OBL_CHAR_SHAPE_ADDR)] =
            obl_create_cshape(database, "Character", 0, no_slots, OBL_CHAR);

    database->fixed[_index_for_fixed(OBL_CHUNK_SHAPE_ADDR)] =
            obl_create_cshape(database, "OblChunk", 0, no_slots, OBL_CHUNK);
    database->fixed[_index_for_fixed(OBL_BOOLEAN_SHAPE_ADDR)] =
            obl_create_cshape(database, "Boolean", 0, no_slots, OBL_BOOLEAN);
    database->fixed[_index_for_fixed(OBL_STUB_SHAPE_ADDR)] =
            obl_create_cshape(database, "OblStub", 0, no_slots, OBL_STUB);
    database->fixed[_index_for_fixed(OBL_ADDRTREEPAGE_SHAPE_ADDR)] =
            obl_create_cshape(database, "OblAddressTreePage", 0, no_slots,
                    OBL_ADDRTREEPAGE);

    /*
     * Allocate the only instances of the other two of the three immutables:
     * true, and false.
     */
    database->fixed[_index_for_fixed(OBL_TRUE_ADDR)] =
            _obl_create_bool(database, 1);
    database->fixed[_index_for_fixed(OBL_FALSE_ADDR)] =
            _obl_create_bool(database, 0);

    /* Set the logical and physical addresses of these objects. */
    for (i = 0; i < OBL_FIXED_SIZE; i++) {
        addr = (obl_logical_address) OBL_FIXED_ADDR_MIN + i;
        database->fixed[i]->physical_address = (obl_physical_address) 0;
        database->fixed[i]->logical_address = addr;
    }

    return 0;
}

static int _index_for_fixed(obl_logical_address addr)
{
    return addr - OBL_FIXED_ADDR_MIN;
}
