/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#include "database.h"

#include "addressmap.h"
#include "allocator.h"
#include "cache.h"
#include "constants.h"
#include "io.h"
#include "log.h"
#include "object.h"
#include "platform.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Internal function prototypes. */

static inline int _is_fixed_address(const obl_logical_address addr);

static int _initialize_fixed_objects(struct obl_database *database);

static inline int _index_for_fixed(obl_logical_address addr);

static void _grow_database(struct obl_database *d);

static void _bootstrap_database(struct obl_database *d);

static void _read_root(struct obl_database *d);

static void _write_root(struct obl_database *d);

/* Error codes: one for each error_code in error.h. */

static char *error_messages[] = {
        "EVERYTHING IS FINE",
        "Unable to allocate an object", "Unable to read file",
        "Unable to open file", "Error during Unicode conversion",
        "Incorrect object storage type", "Bad argument length",
        "Missing a critical system object", "Database must be open"
};

/* Addresses of the elements in obl_root. */

#define ADDRMAP_ADDR 1
#define ALLOCATOR_ADDR 2
#define NAMEMAP_ADDR 3
#define SHAPEMAP_ADDR 4

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
    database->growth_size = (obl_uint) DEFAULT_GROWTH_SIZE;

    /* Initialize +root+ to OBL_UNASSIGNED until opened. */
    database->root.address_map_addr = OBL_PHYSICAL_UNASSIGNED;
    database->root.allocator_addr = OBL_PHYSICAL_UNASSIGNED;
    database->root.name_map_addr = OBL_PHYSICAL_UNASSIGNED;
    database->root.shape_map_addr = OBL_PHYSICAL_UNASSIGNED;
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
    database->content_size = (obl_uint) 0;

    return database;
}

int obl_open_database(struct obl_database *d, int allow_creation)
{
    int fd, flags;
    struct stat buffer;

    flags = O_RDWR;
    if (allow_creation) {
        flags |= O_CREAT;
    }

    fd = open(d->filename, flags, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        /* Unable to open the database file. */
        obl_report_errorf(d, OBL_UNABLE_TO_OPEN_FILE,
                "Unable to open file <%s>: %s",
                d->filename,
                strerror(errno));
        return 1;
    }

    if( fstat(fd, &buffer) ) {
        /* Unable to stat database file. */
        close(fd);
        obl_report_errorf(d, OBL_UNABLE_TO_OPEN_FILE,
                "Unable to stat file <%s>: %s",
                d->filename,
                strerror(errno));
        return 1;
    }

    d->content_size = (obl_uint) (buffer.st_size / sizeof(obl_uint));
    d->content = (obl_uint*) mmap(NULL, d->content_size,
            PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    if (d->content_size < d->growth_size) {
        _grow_database(d);
        _bootstrap_database(d);
    }

    _read_root(d);

    return 0;
}

int obl_is_open(struct obl_database *d)
{
    return d->content != NULL && d->content_size > 0;
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
    obl_physical_address addr;

    /* Check for fixed addresses first. */
    if (_is_fixed_address(address)) {
        return database->fixed[_index_for_fixed(address)];
    }

    /* Check for a cache hit. */
    o = obl_cache_get(database->cache, address);
    if (o != NULL) {
        return o;
    }

    /* Look up the physical address. */
    addr = obl_address_lookup(database, address);
    if (addr == OBL_PHYSICAL_UNASSIGNED) {
        return obl_nil(database);
    }

    return obl_read_object(database, database->content, addr, depth);
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

void obl_close_database(struct obl_database *database)
{
    if (!obl_is_open(database)) {
        OBL_WARN(database, "Database already closed.");
        return ;
    }

    munmap(database->content, database->content_size * sizeof(obl_uint));

    database->content = (obl_uint*) 0;
    database->content_size = (obl_uint) 0;
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

void _obl_write(struct obl_object *o)
{
    struct obl_database *d;

    d = o->database;
    if (!obl_is_open(d)) {
        obl_report_error(d, OBL_DATABASE_NOT_OPEN, NULL);
        return ;
    }

    if (o->logical_address == OBL_LOGICAL_UNASSIGNED) {
        o->logical_address = obl_allocate_logical(d);
    }

    if (o->physical_address == OBL_PHYSICAL_UNASSIGNED) {
        obl_uint size, extent;

        size = obl_object_wordsize(o);
        o->physical_address = obl_allocate_physical(d, size);

        extent = (obl_uint) (o->physical_address) + size;
        if (extent >= d->content_size) {
            _grow_database(d);
        }

        obl_address_assign(d, o->logical_address, o->physical_address);
    }

    obl_write_object(o, d->content);
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
    char *allocator_slots[2] = {
            "next_logical", "next_physical"
    };
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
    database->fixed[_index_for_fixed(OBL_ALLOCATOR_SHAPE_ADDR)] =
            obl_create_cshape(database, "OblAllocator", 2, allocator_slots,
                    OBL_SLOTTED);

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

static void _grow_database(struct obl_database *d)
{
    FILE *fd;

    if (obl_is_open(d)) {
        obl_close_database(d);
    }

    fd = fopen(d->filename, "rb+");
    fseek(fd, (d->growth_size * sizeof(obl_int)) - 1, SEEK_END);
    fputc('\0', fd);
    fclose(fd);

    obl_open_database(d, 0);
}

static void _bootstrap_database(struct obl_database *d)
{
    /* obl\0 in hex. */
    const obl_uint magic = 0x6F626C00;
    struct obl_object *treepage, *allocator;
    struct obl_object *next_physical, *next_logical;
    obl_logical_address current_logical;
    obl_physical_address current_physical;

    /* Write the "magic word" at address 0. */
    d->content[0] = writable_uint(magic);

    /* Logical 0 is OBL_LOGICAL_UNASSIGNED. */
    current_logical = (obl_logical_address) 1;

    /*
     * Physical 0 is the magic word (and OBL_PHYSICAL_UNASSIGNED).
     * 1 - 4 are occupied by root.  Allocation begins at 5.
     */
    current_physical = (obl_physical_address) 5;

    /* First: the allocator. */
    allocator = obl_create_slotted(obl_at_address(d, OBL_ALLOCATOR_SHAPE_ADDR));
    allocator->logical_address = current_logical;
    allocator->physical_address = current_physical;
    current_logical++;
    current_physical += obl_object_wordsize(allocator);

    /* Next, its physical and logical word containers. */
    next_physical = obl_create_integer(d, (obl_int) 0);
    next_physical->logical_address = current_logical;
    next_physical->physical_address = current_physical;
    obl_slotted_atcnamed_put(allocator, "next_physical", next_physical);
    current_logical++;
    current_physical += obl_object_wordsize(next_physical);

    next_logical = obl_create_integer(d, (obl_int) 0);
    next_logical->logical_address = current_logical;
    next_logical->physical_address = current_physical;
    obl_slotted_atcnamed_put(allocator, "next_logical", next_logical);
    current_logical++;
    current_physical += obl_object_wordsize(next_logical);

    /*
     * The first leaf of the address map.  Address map pages are not assigned
     * logical addresses.
     */
    treepage = obl_create_addrtreepage(d, 0);
    treepage->physical_address = current_physical;
    current_physical += obl_object_wordsize(treepage);

    /* Place assigned addresses within the root and the allocator's state. */
    d->root.address_map_addr = treepage->physical_address;
    d->root.allocator_addr = allocator->logical_address;
    d->root.name_map_addr = OBL_LOGICAL_UNASSIGNED;
    d->root.shape_map_addr = OBL_LOGICAL_UNASSIGNED;
    obl_integer_set(next_physical, (int) current_physical);
    obl_integer_set(next_logical, (int) current_logical);

    /* Write everything so far. */
    _write_root(d);
    obl_write_object(allocator, d->content);
    obl_write_object(next_physical, d->content);
    obl_write_object(next_logical, d->content);
    obl_write_object(treepage, d->content);

    /* Write the address assignments into the address map. */
    obl_address_assign(d,
            allocator->logical_address, allocator->physical_address);
    obl_address_assign(d,
            next_physical->logical_address, next_physical->physical_address);
    obl_address_assign(d,
            next_logical->logical_address, next_logical->physical_address);

    /* Cache temporary objects. */
    obl_cache_insert(d->cache, allocator);
    obl_cache_insert(d->cache, next_physical);
    obl_cache_insert(d->cache, next_logical);
}

static void _read_root(struct obl_database *d)
{
    d->root.address_map_addr = readable_physical(d->content[ADDRMAP_ADDR]);
    d->root.allocator_addr = readable_logical(d->content[ALLOCATOR_ADDR]);
    d->root.name_map_addr = readable_logical(d->content[NAMEMAP_ADDR]);
    d->root.shape_map_addr = readable_logical(d->content[SHAPEMAP_ADDR]);

    d->root.dirty = 0;
}

static void _write_root(struct obl_database *d)
{
    d->content[ADDRMAP_ADDR] = writable_physical(d->root.address_map_addr);
    d->content[ALLOCATOR_ADDR] = writable_logical(d->root.allocator_addr);
    d->content[NAMEMAP_ADDR] = writable_logical(d->root.shape_map_addr);
    d->content[SHAPEMAP_ADDR] = writable_logical(d->root.name_map_addr);

    d->root.dirty = 0;
}
