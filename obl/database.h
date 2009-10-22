/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "log.h"
#include "platform.h"

/* Defined in cache.h */
struct obl_cache;

/* Defined in object.h */
struct obl_object;

/*
 * Fixed allocation.  These logical addresses will always resolve to universally
 * accessible, constant objects that do not reside in the database.
 */
#define OBL_FIXED_SIZE 15

typedef enum
{
    /* A useful constant marking the lowest fixed address. */
    OBL_FIXED_ADDR_MIN = OBL_ADDRESS_MAX - OBL_FIXED_SIZE + 1,

    /* Special constants: nil, true, and false. */
    OBL_NIL_ADDR = OBL_FIXED_ADDR_MIN, /* 0xfff1 */
    OBL_TRUE_ADDR,                     /* 0xfff2 */
    OBL_FALSE_ADDR,                    /* 0xfff3 */

    /* The primitive Shape objects. */
    OBL_INTEGER_SHAPE_ADDR,            /* 0xfff4 */
    OBL_FLOAT_SHAPE_ADDR,              /* 0xfff5 */
    OBL_DOUBLE_SHAPE_ADDR,             /* 0xfff6 */
    OBL_CHAR_SHAPE_ADDR,               /* 0xfff7 */
    OBL_STRING_SHAPE_ADDR,             /* 0xfff8 */

    /* Built-in collection Shape objects. */
    OBL_FIXED_SHAPE_ADDR,              /* 0xfff9 */
    OBL_CHUNK_SHAPE_ADDR,              /* 0xfffa */
    OBL_ADDRTREEPAGE_SHAPE_ADDR,       /* 0xfffb */
    OBL_ALLOCATOR_SHAPE_ADDR,          /* 0xfffc */

    /* Virtual Shape objects. */
    OBL_NIL_SHAPE_ADDR,                /* 0xfffd */
    OBL_BOOLEAN_SHAPE_ADDR,            /* 0xfffe */
    OBL_STUB_SHAPE_ADDR                /* 0xffff */
} obl_fixed_address;

/*
 * The available error codes.  Each error code should correspond to one
 * exception class in each language's binding.
 */
typedef enum
{
    OBL_OK,
    OBL_OUT_OF_MEMORY,
    OBL_UNABLE_TO_READ_FILE,
    OBL_UNABLE_TO_OPEN_FILE,
    OBL_CONVERSION_ERROR,
    OBL_WRONG_STORAGE,
    OBL_ARGUMENT_SIZE,
    OBL_MISSING_SYSTEM_OBJECT,
    OBL_DATABASE_NOT_OPEN,
    OBL_INVALID_INDEX
} error_code;

/*
 * A structure for storing information about an error that's occurred.
 */
struct error {
    char *message;
    error_code code;
};

/*
 * The root object of the database, which always resides at physical address 0
 * and has no mapping within the logical address space.  Root contains direct
 * references to the four structures which need to be present for the database
 * to operate.
 */
struct obl_root {

    /*
     * This physical address must point to the root of the structure of
     * OBL_ADDRTREEPAGE objects that map logical addresses to physical addresses.
     * See +addressmap.h+.
     */
    obl_physical_address address_map_addr;

    /*
     * This address must point to the saved state of the allocator, an
     * obl_object that assigns free logical and physical addresses to newly
     * created objects.  See +allocator.h+.
     */
    obl_logical_address allocator_addr;

    /*
     * The object at this physical address is the root of a dictionary of
     * OBL_SHAPE objects by OBL_STRING name.
     */
    obl_logical_address shape_map_addr;

    /*
     * The dictionary at this physical address contains user-defined entry
     * points to structures of persisted data.
     */
    obl_logical_address name_map_addr;

    /*
     * This structure is not an obl_object and cannot be added to a
     * transaction's write set.  This dirty flag indicates that one of these
     * addresses has changed and indicates that the root object must be
     * rewritten.
     */
    int dirty;
};

/*
 * ObjectLite interface layer.
 */
struct obl_database {

    /* Location of the persisted database. */
    const char *filename;

    /*
     * Object cache to prevent unnecessary address translations
     * and support self-referential object structures.
     */
    struct obl_cache *cache;

    /* Fixed object space. */
    struct obl_object **fixed;

    /* Logging and error structures. */
    struct obl_log_configuration log_config;
    struct error last_error;

    /* Stubbing control. */
    int default_stub_depth;

    /*
     * Grow the database file by this many obl_uint units each time growth
     * is triggered.
     */
    obl_uint growth_size;

    /* Root storage.  Initialized during open. */
    struct obl_root root;

    /* The memory-mapped contents of the database file. */
    obl_uint *content;

    /*
     * If opened, contains the mapped extent of +content+ in sizeof(obl_uint)
     * units.
     */
    obl_uint content_size;
};

/*
 * Allocate structures for a new ObjectLite database interface layer, using all
 * of the default settings.  Database objects created in this manner must be
 * destroyed by +obl_destroy_database()+.
 */
struct obl_database *obl_create_database(const char *filename);

/*
 * Open the database file using the existing settings within +d+.  If
 * +allow_creation+ is 1, bootstrap the database if it does not already exist.
 * Returns 0 on a successful open.  Logs a warning and returns 1 on an
 * unsuccessful open.
 */
int obl_open_database(struct obl_database *d, int allow_creation);

/*
 * Return 1 if +d+ has been opened successfully, or 0 if it has not.
 */
int obl_is_open(struct obl_database *d);

/*
 * The most basic query: return an object that lives at a known logical address.
 * Use the default stub depth as configures in +database+.
 */
struct obl_object *obl_at_address(struct obl_database *database,
        const obl_logical_address address);

/*
 * Retrieve an object to a specified stub depth.
 */
struct obl_object *obl_at_address_depth(struct obl_database *database,
        const obl_logical_address address, int depth);

/*
 * Return the single instance of +nil+.
 */
struct obl_object *obl_nil(struct obl_database *database);

/*
 * Return the single object representing truth.
 */
struct obl_object *obl_true(struct obl_database *database);

/*
 * Return the single object representing falsehood.
 */
struct obl_object *obl_false(struct obl_database *database);

/*
 * Close the opened database file.
 */
void obl_close_database(struct obl_database *database);

/*
 * Deallocate all of the resources associated with an ObjectLite database.
 */
void obl_destroy_database(struct obl_database *database);

/*
 * Return TRUE if +database+ has no active error code, or FALSE if it does.  The
 * error status of a database may be reset using the +obl_clear_error()+
 * function.
 */
int obl_database_ok(const struct obl_database *database);

/*
 * Unset any active error codes in +database+.
 */
void obl_clear_error(struct obl_database *database);

/*
 * Set the active error code in +database+.  If +message+ is NULL, a default
 * message will be used.
 */
void obl_report_error(struct obl_database *database, error_code code,
        const char *message);

/*
 * Format an error message with variables a la +sprintf()+ and friends.
 */
void obl_report_errorf(struct obl_database *database, error_code code,
        const char *format, ...);

/*
 * Allocate any necessary addresses, grow the database file if necessary, then
 * serialize the object +o+ to the file using the functionality provided by
 * +io.h+.
 *
 * This function performs primitive I/O and should only be called by internal
 * ObjectLite functions while a transaction is active and the database is
 * properly locked.
 */
void _obl_write(struct obl_object *o);

#endif
