/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#include "database.h"

#include "storage/object.h"
#include "addressmap.h"
#include "allocator.h"
#include "constants.h"
#include "log.h"
#include "platform.h"
#include "session.h"
#include "set.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Internal function prototypes. */

static int _initialize_fixed_space();

static void _destroy_fixed_space();

static inline int _index_for_fixed(obl_logical_address addr);

static int _obl_map_database(struct obl_database *d);

static int _obl_unmap_database(struct obl_database *d);

static void _grow_database(struct obl_database *d);

static void _bootstrap_database(struct obl_database *d);

static void _read_root(struct obl_database *d);

static void _write_root(struct obl_database *d);

/** Error codes: one for each obl_error_code in log.h. */
static char *error_messages[] = {
        "EVERYTHING IS FINE",
        "Unable to allocate an object",
        "Unable to read file",
        "Unable to open file",
        "Error during Unicode conversion",
        "Incorrect object storage type",
        "Bad argument length",
        "Missing a critical system object",
        "Database must be open",
        "Invalid index",
        "Invalid address",
        "An attempt was made to begin a transaction while one was already in progress"
};

/** Storage for fixed space, shared by all active databases. */
static struct obl_object *fixed_space[OBL_FIXED_SIZE] = { 0 };

/**
 * A magic number used to prefix a valid database.  The string "obl\0" in hex.
 */
static const obl_uint magic = 0x6F626C00;

/* Addresses of the elements in obl_root. */

#define ADDRMAP_ADDR 1
#define ALLOCATOR_ADDR 2
#define NAMEMAP_ADDR 3
#define SHAPEMAP_ADDR 4

/* External functions definitions. */

int obl_startup()
{
    _initialize_fixed_space();

    return 0;
}

int obl_shutdown()
{
    _destroy_fixed_space();

    return 0;
}

struct obl_database *obl_open_database(struct obl_database_config *config)
{
    struct obl_database *d = NULL;
    struct obl_database_config *conf;

    /* Sanity-check the user-provided configuration. */
    if (config->zero_check) {
        OBL_ERROR(d,
                "The database configuration structure is "
                "not zero-initialized.");
        return NULL;
    }

    d = malloc(sizeof(struct obl_database));
    if (d == NULL) {
        OBL_ERROR(d, "Unable to allocate the database.");
        return NULL;
    }
    d->configuration = *config;

    /* Accept defaults. */
    conf = &d->configuration;
    if (conf->default_stub_depth == 0)
        conf->default_stub_depth = DEFAULT_STUB_DEPTH;
    if (conf->growth_size == 0)
        conf->growth_size = DEFAULT_GROWTH_SIZE;
    if (conf->log_level == L_DEFAULT)
        conf->log_level = L_NOTICE;

    /* Initialize the error flags. */
    d->error_message = NULL;
    d->error_code = OBL_OK;

    /* Prepare +root+ as OBL_UNASSIGNED until the actual file is mapped. */
    d->root.address_map_addr = OBL_PHYSICAL_UNASSIGNED;
    d->root.allocator_addr = OBL_PHYSICAL_UNASSIGNED;
    d->root.name_map_addr = OBL_PHYSICAL_UNASSIGNED;
    d->root.shape_map_addr = OBL_PHYSICAL_UNASSIGNED;
    d->root.dirty = 0;

    /* Prepare the content pointer to be appropriately empty. */
    d->content = NULL;
    d->content_size = (obl_uint) 0;

    /* Initialize the content lock. */
    sem_init(&d->content_mutex, 0, 1);

    if (_obl_map_database(d)) {
        sem_destroy(&d->content_mutex);
        free(d);
        return NULL;
    }

    if (d->content_size == 0) {
        _grow_database(d);
    }

    if (readable_uint(d->content[0]) != magic) {
        OBL_INFO(d, "Bootstrapping the database.");
        _bootstrap_database(d);
    }

    _read_root(d);

    return d;
}

struct obl_database *obl_open_defdatabase(const char *filename)
{
    struct obl_database_config conf = { 0 };
    conf.filename = filename;

    return obl_open_database(&conf);
}

struct obl_object *obl_nil()
{
    return _obl_at_fixed_address(OBL_NIL_ADDR);
}

struct obl_object *obl_true()
{
    return _obl_at_fixed_address(OBL_TRUE_ADDR);
}

struct obl_object *obl_false()
{
    return _obl_at_fixed_address(OBL_FALSE_ADDR);
}

void obl_close_database(struct obl_database *d)
{
    _obl_unmap_database(d);

    if (d->error_message != NULL ) {
        free(d->error_message);
    }

    sem_destroy(&d->content_mutex);

    free(d);
}

int obl_database_ok(const struct obl_database *d)
{
    return d->error_code == OBL_OK;
}

void obl_clear_error(struct obl_database *d)
{
    if (d->error_message != NULL) {
        free(d->error_message);
    }
    d->error_message = NULL;
    d->error_code = OBL_OK;
}

void obl_report_error(struct obl_database *d, obl_error_code code,
        const char *message)
{
    const char *real_message;
    char *buffer;
    size_t message_size;

    if (message != NULL) {
        real_message = message;
    } else {
        real_message = error_messages[code];
    }

    OBL_ERROR(d, real_message);

    if (d == NULL) {
        fprintf(stderr, "Unable to report error \"%s\":\n", real_message);
        fprintf(stderr, "No database structure available to report it in.\n");
        return;
    }

    message_size = strlen(real_message) + 1;
    buffer = malloc(message_size);
    memcpy(buffer, real_message, message_size);

    d->error_message = buffer;
    d->error_code = code;
}

void obl_report_errorf(struct obl_database *d, obl_error_code code,
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
    buffer = malloc(required_size);
    vsnprintf(buffer, required_size, format, args);
    va_end(args);

    obl_report_error(d, code, buffer);

    free(buffer);
}

struct obl_object *_obl_at_fixed_address(obl_logical_address address)
{
    if (!IS_FIXED_ADDR(address)) {
        obl_report_errorf(NULL, OBL_INVALID_ADDRESS,
                "Expected an address in fixed space, received 0x%lu.",
                (unsigned long) address);
        return NULL;
    }

    return fixed_space[_index_for_fixed(address)];
}

void _obl_write(struct obl_object *o)
{
    struct obl_session *s = o->session;
    struct obl_database *d = NULL;

    if (s == NULL) {
        OBL_ERROR(d, "_obl_write called with an object that has not been "
                "assigned to a session yet.");
        return ;
    }

    d = s->database;
    if (d->content == NULL) {
        obl_report_error(d, OBL_DATABASE_NOT_OPEN, NULL);
        return ;
    }

    if (o->logical_address == OBL_LOGICAL_UNASSIGNED) {
        o->logical_address = obl_allocate_logical(s);
    }

    if (o->physical_address == OBL_PHYSICAL_UNASSIGNED) {
        obl_uint size, extent;

        size = obl_object_wordsize(o);
        o->physical_address = obl_allocate_physical(s, size);

        extent = (obl_uint) (o->physical_address) + size;
        if (extent >= d->content_size) {
            _grow_database(d);
        }

        obl_address_assign(s, o->logical_address, o->physical_address);
    }

    obl_write_object(o, d->content);
}

void _obl_database_release(struct obl_object *o)
{
    struct obl_database *d;

    if (o->session == NULL) return ;
    d = o->session->database;
}

/* Internal function implementations. */

static int _initialize_fixed_space()
{
    int i;
    obl_logical_address addr;
    char *no_slots[0];
    char *allocator_slots[2] = {
            "next_logical", "next_physical"
    };
    struct obl_object *fixed_shape, *string_shape, *undefined_shape;
    struct obl_object *nil;

    /* If fixed space has already been allocated, don't allocate it twice. */
    for (i = 0; i < OBL_FIXED_SIZE; i++) {
        if (fixed_space[i] != NULL) {
            return 0;
        }
    }

    /*
     * The FixedCollection, String, Undefined shapes and Nil object are used
     * inside of shape objects (including their own).  Create these objects
     * first and manually correct the structures of their shape members.
     */
    fixed_shape = obl_create_cshape("FixedCollection", 0, no_slots, OBL_FIXED);
    string_shape = obl_create_cshape("String", 0, no_slots, OBL_STRING);
    undefined_shape = obl_create_cshape("Undefined", 0, no_slots, OBL_NIL);
    nil = _obl_create_nil();

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

    fixed_space[_index_for_fixed(OBL_FIXED_SHAPE_ADDR)] = fixed_shape;
    fixed_space[_index_for_fixed(OBL_STRING_SHAPE_ADDR)] = string_shape;
    fixed_space[_index_for_fixed(OBL_NIL_SHAPE_ADDR)] = undefined_shape;
    fixed_space[_index_for_fixed(OBL_NIL_ADDR)] = nil;

    /*
     * Allocate the rest of the fixed-space shape objects.  These are shape
     * objects for ObjectLite primitives.
     */
    fixed_space[_index_for_fixed(OBL_INTEGER_SHAPE_ADDR)] =
            obl_create_cshape("Integer", 0, no_slots, OBL_INTEGER);
    fixed_space[_index_for_fixed(OBL_FLOAT_SHAPE_ADDR)] =
            obl_create_cshape("Float", 0, no_slots, OBL_FLOAT);
    fixed_space[_index_for_fixed(OBL_DOUBLE_SHAPE_ADDR)] =
            obl_create_cshape("Double", 0, no_slots, OBL_DOUBLE);
    fixed_space[_index_for_fixed(OBL_CHAR_SHAPE_ADDR)] =
            obl_create_cshape("Character", 0, no_slots, OBL_CHAR);
    fixed_space[_index_for_fixed(OBL_CHUNK_SHAPE_ADDR)] =
            obl_create_cshape("OblChunk", 0, no_slots, OBL_CHUNK);
    fixed_space[_index_for_fixed(OBL_BOOLEAN_SHAPE_ADDR)] =
            obl_create_cshape("Boolean", 0, no_slots, OBL_BOOLEAN);
    fixed_space[_index_for_fixed(OBL_STUB_SHAPE_ADDR)] =
            obl_create_cshape("OblStub", 0, no_slots, OBL_STUB);
    fixed_space[_index_for_fixed(OBL_ADDRTREEPAGE_SHAPE_ADDR)] =
            obl_create_cshape("OblAddressTreePage", 0, no_slots,
                    OBL_ADDRTREEPAGE);
    fixed_space[_index_for_fixed(OBL_ALLOCATOR_SHAPE_ADDR)] =
            obl_create_cshape("OblAllocator", 2, allocator_slots,
                    OBL_SLOTTED);

    /*
     * Allocate the only instances of the other two of the three immutables:
     * true and false.
     */
    fixed_space[_index_for_fixed(OBL_TRUE_ADDR)] = _obl_create_bool(1);
    fixed_space[_index_for_fixed(OBL_FALSE_ADDR)] = _obl_create_bool(0);

    /* Set the logical and physical addresses of these objects. */
    for (i = 0; i < OBL_FIXED_SIZE; i++) {
        if (fixed_space[i] == NULL) {
            return 1;
        }

        addr = (obl_logical_address) OBL_FIXED_ADDR_MIN + i;
        fixed_space[i]->physical_address = OBL_PHYSICAL_UNASSIGNED;
        fixed_space[i]->logical_address = addr;
    }

    return 0;
}

static void _destroy_fixed_space()
{
    int i;
    struct obl_object *o, *nil;

    nil = obl_nil();
    for (i = 0; i < OBL_FIXED_SIZE; i++) {
        o = fixed_space[i];
        if (o != nil) {
            if (o->shape == nil) {
                obl_destroy_cshape(o);
            } else {
                obl_destroy_object(o);
            }

            fixed_space[i] = NULL;
        }
    }

    obl_destroy_object(nil);
}

static inline int _index_for_fixed(obl_logical_address addr)
{
    return addr - OBL_FIXED_ADDR_MIN;
}

static int _obl_map_database(struct obl_database *d)
{
    int fd, flags;
    struct stat buffer;

    if (d->configuration.filename == NULL) {
        /* Prepare an in-memory database on the heap. */

        d->content_size = (obl_uint) d->configuration.growth_size;
        d->content = malloc(sizeof(obl_uint) * d->content_size);
        memset(d->content, 0, sizeof(obl_uint) * d->content_size);

        if (d->content == NULL) {
            d->content_size = (obl_uint) 0;
            obl_report_error(d, OBL_OUT_OF_MEMORY,
                    "Unable to allocate enough space for an in-memory "
                    "database.");
            return 1;
        }

        return 0;
    }

    flags = O_RDWR;
    if (! d->configuration.prohibit_creation) {
        flags |= O_CREAT;
    }

    fd = open(d->configuration.filename, flags, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        /* Unable to open the database file. */
        obl_report_errorf(d, OBL_UNABLE_TO_OPEN_FILE,
                "Unable to open file <%s>: %s",
                d->configuration.filename,
                strerror(errno));
        return 1;
    }

    if (fstat(fd, &buffer)) {
        /* Unable to stat database file. */
        close(fd);
        obl_report_errorf(d, OBL_UNABLE_TO_OPEN_FILE,
                "Unable to stat file <%s>: %s",
                d->configuration.filename,
                strerror(errno));
        return 1;
    }

    d->content_size = (obl_uint) (buffer.st_size / sizeof(obl_uint));
    d->content = (obl_uint*) mmap(NULL, d->content_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    return 0;
}

static int _obl_unmap_database(struct obl_database *d)
{
    if (d->configuration.filename == NULL) {
        /* Free heap space. */

        free(d->content);
        d->content = NULL;
        d->content_size = (obl_uint) 0;

        return 0;
    }

    munmap(d->content, d->content_size * sizeof(obl_uint));

    d->content = NULL;
    d->content_size = (obl_uint) 0;

    return 0;
}

static void _grow_database(struct obl_database *d)
{
    FILE *fd;

    sem_wait(&d->content_mutex);

    if (d->configuration.filename == NULL) {
        /* Allocate additional space on the heap. */

        d->content_size = d->content_size + d->configuration.growth_size;
        d->content = realloc(d->content, d->content_size);

        if (d->content == NULL) {
            obl_report_errorf(d, OBL_OUT_OF_MEMORY,
                    "Unable to grow an in-memory database to <%lu> bytes.",
                    d->content_size);
            d->content_size = (obl_uint) 0;
        }

        sem_post(&d->content_mutex);
        return ;
    }

    if (d->content != NULL) {
        _obl_unmap_database(d);
    }

    fd = fopen(d->configuration.filename, "rb+");
    fseek(fd, d->configuration.growth_size - 1, SEEK_END);
    fputc('\0', fd);
    fclose(fd);

    _obl_map_database(d);
    sem_post(&d->content_mutex);
}

static void _bootstrap_database(struct obl_database *d)
{
    struct obl_session *s;
    struct obl_object *treepage, *allocator;
    struct obl_object *next_physical, *next_logical;
    obl_logical_address current_logical;
    obl_physical_address current_physical;

    s = obl_create_session(d);
    if (s == NULL) return ;

    /* Logical 0 is OBL_LOGICAL_UNASSIGNED, so start at Logical 1. */
    current_logical = (obl_logical_address) 1;

    /*
     * Physical 0 is the magic word (and OBL_PHYSICAL_UNASSIGNED).
     * 1 - 4 are occupied by root.  Allocation begins at 5.
     */
    current_physical = (obl_physical_address) 5;

    /* First: the allocator. */
    allocator = obl_create_slotted(obl_at_address(s, OBL_ALLOCATOR_SHAPE_ADDR));
    allocator->logical_address = current_logical;
    allocator->physical_address = current_physical;
    current_logical++;
    current_physical += obl_object_wordsize(allocator);

    /* Next, its physical and logical word containers. */
    next_physical = obl_create_integer((obl_int) 0);
    next_physical->logical_address = current_logical;
    next_physical->physical_address = current_physical;
    obl_slotted_atcnamed_put(allocator, "next_physical", next_physical);
    current_logical++;
    current_physical += obl_object_wordsize(next_physical);

    next_logical = obl_create_integer((obl_int) 0);
    next_logical->logical_address = current_logical;
    next_logical->physical_address = current_physical;
    obl_slotted_atcnamed_put(allocator, "next_logical", next_logical);
    current_logical++;
    current_physical += obl_object_wordsize(next_logical);

    /*
     * The first leaf of the address map.  Address map pages are not assigned
     * logical addresses.
     */
    treepage = obl_create_addrtreepage(0);
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
    obl_address_assign(s, allocator->logical_address,
            allocator->physical_address);
    obl_address_assign(s, next_physical->logical_address,
            next_physical->physical_address);
    obl_address_assign(s, next_logical->logical_address,
            next_logical->physical_address);

    /* Store temporary objects in the read set. */
    obl_set_insert(s->read_set, allocator);
    obl_set_insert(s->read_set, next_physical);
    obl_set_insert(s->read_set, next_logical);

    obl_destroy_session(s);

    /*
     * Write the "magic word" at address 0 to indicate a successful
     * bootstrapping.
     */
    d->content[0] = writable_uint(magic);
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
