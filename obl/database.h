/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file database.h
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

/* Defined in session.h */
struct obl_session;

/* Defined in session.h */
struct obl_session_list;

/** Size of fixed space. */
#define OBL_FIXED_SIZE 15

/**
 * Fixed allocation.  These logical addresses will always resolve to universally
 * accessible, constant objects that do not reside in the database.
 */
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

/**
 * A handy macro to determine if a logical address falls within fixed space.
 */
#define IS_FIXED_ADDR(addr) ((addr) >= OBL_FIXED_ADDR_MIN)

/**
 * A user-editable structure that customizes and optimizes the behaviour of
 * an obl_database.  Zero-initialize it to accept default options.
 */
struct obl_database_config
{
    /**
     * The *.obl database filename.  This setting only has an effect on the
     * obl_open_database() call (unlike other settings, changing it within an
     * opened database has no effect).
     *
     * Default: NULL, which operates as an in-memory database allocated on
     * the heap.
     */
    const char *filename;

    /**
     * If zero, obl_open_database() will create the file called filename if
     * no such file exists.  If nonzero, only an existing file will be opened.
     *
     * Default: allow file creation.
     */
    int prohibit_creation;

    /**
     * The number of bytes to grow the database file each time it allocates all
     * available space.  Smaller values will create more compact storage, but
     * larger values will reduce the number of growth operations that need to
     * be performed (which are very expensive).
     *
     * Default: 1024.
     */
    int growth_size;

    /**
     * If specified, ObjectLite will log messages to the specified file.
     *
     * Default: NULL, which outputs messages to stderr.
     */
    char *log_filename;

    /**
     * Filter log messages by severity.  L_DEBUG shows all messages; L_NONE
     * shows nothing.
     *
     * Default: L_NOTICE.
     */
    enum obl_log_level log_level;

    /**
     * When faulting objects from the database file, this many reference levels
     * will be followed.
     *
     * Default: 4.
     */
    int default_stub_depth;

    /**
     * A field to verify that you've properly zero-initialized the structure.
     */
    int zero_check;
};

/**
 * The root object of the database, which always resides at physical address 0
 * and has no mapping within the logical address space.  Root contains direct
 * references to the four structures which need to be present for the database
 * to operate.
 */
struct obl_root {

    /**
     * This physical address must point to the root of the structure of
     * OBL_ADDRTREEPAGE objects that map logical addresses to physical addresses.
     * See addressmap.h.
     */
    obl_physical_address address_map_addr;

    /**
     * This address must point to the saved state of the allocator, an
     * obl_object that assigns free logical and physical addresses to newly
     * created objects.  See allocator.h.
     */
    obl_logical_address allocator_addr;

    /**
     * The object at this physical address is the root of a dictionary of
     * OBL_SHAPE objects by OBL_STRING name.
     */
    obl_logical_address shape_map_addr;

    /**
     * The dictionary at this physical address contains user-defined entry
     * points to structures of persisted data.
     */
    obl_logical_address name_map_addr;

    /**
     * This structure is not an obl_object and cannot be added to a
     * transaction's write set.  This dirty flag indicates that one of these
     * addresses has changed and indicates that the root object must be
     * rewritten.
     */
    int dirty;
};

/**
 * ObjectLite interface layer.
 */
struct obl_database
{
    /** The database configuration provided to obl_open_database(). */
    struct obl_database_config configuration;

    /** The last error message, heap-allocated.  NULL if all is well. */
    char *error_message;

    /** The last error code.  OBL_OK if all is well. */
    enum obl_error_code error_code;

    /** Root storage.  Initialized during open. */
    struct obl_root root;

    /** The memory-mapped contents of the database file. */
    obl_uint *content;

    /**
     * If opened, contains the mapped extent of content in sizeof(obl_uint)
     * units.
     */
    obl_uint content_size;

    /** A semaphore to make database content operations atomic. */
    sem_t content_mutex;

    /** A singly-linked list of currently active sessions. */
    struct obl_session_list *session_list;

    /** A semaphore to protect access to the session list. */
    sem_t session_list_mutex;
};

/**
 * Allocate and prepare global internal ObjectLite resources.  Call this
 * function before you invoke any other obl functions, and before you create
 * multiple threads.
 */
int obl_startup();

/**
 * Clean up global internal ObjectLite resources.  Call this function before
 * your program terminates, but after you're done calling any and all
 * obl functions, and after you've joined any and all threads.
 */
int obl_shutdown();

/**
 * Create and open an obl_database using the provided configuration information.
 * Databases created with this function must be closed by a call to
 * obl_close_database().
 *
 * @param config Zero-initialize this structure, then customize the settings
 *      as you see fit.
 * @return A pointer to the newly allocated obl_database.  Returns NULL if there
 *      is an allocation problem.
 */
struct obl_database *obl_open_database(struct obl_database_config *config);

/**
 * Open a database accepting all default obl_database_config options, except
 * for the name of the .obl database file.
 *
 * @param filename
 */
struct obl_database *obl_open_defdatabase(const char *filename);

/**
 * Return the single instance of nil.
 */
struct obl_object *obl_nil();

/**
 * Return the single object representing truth.
 */
struct obl_object *obl_true();

/**
 * Return the single object representing falsehood.
 */
struct obl_object *obl_false();

/**
 * Close an opened database file and dispose of any resources associated with
 * it.  Closes any active obl_session and obl_transaction objects associated
 * with it.
 *
 * @param d A database, as allocated by obl_open_database() or
 *      obl_open_defdatabase().
 */
void obl_close_database(struct obl_database *d);

/**
 * Deallocate all of the resources associated with an ObjectLite database.
 */
void obl_destroy_database(struct obl_database *d);

/**
 * Return TRUE if database has no active error code, or FALSE if it does.  The
 * error status of a database may be reset using the obl_clear_error()
 * function.
 */
int obl_database_ok(const struct obl_database *d);

/**
 * Unset any active error codes in database.
 */
void obl_clear_error(struct obl_database *d);

/**
 * Set the active error code in database.  If message is NULL, a default
 * message will be used.
 */
void obl_report_error(struct obl_database *d, enum obl_error_code code,
        const char *message);

/**
 * Format an error message with variables a la sprintf() and friends.
 */
void obl_report_errorf(struct obl_database *d, enum obl_error_code code,
        const char *format, ...);

/**
 * Retrieve an object directly from fixed space.  For internal use only.
 *
 * @param address A logical address within fixed space.
 * @return The obl_object from fixed space.  Logs an error and returns NULL if
 *      the address is invalid.
 */
struct obl_object *_obl_at_fixed_address(obl_logical_address address);

/**
 * Allocate and map logical and physical addresses to an object if necessary.
 * This method can perform raw database I/O, even including database growth. For
 * internal use only.
 *
 * @param o The object that is (possibly) missing an address.
 * @return 1 if a new logical address was assigned, 0 otherwise.
 */
int _obl_assign_addresses(struct obl_object *o);

/**
 * Allocate any necessary addresses, grow the database file if necessary, then
 * serialize the object o to the file using the functionality provided by
 * io.h.
 *
 * This function performs primitive I/O and should only be called by internal
 * ObjectLite functions while a transaction is active and the database is
 * properly locked.
 *
 * @param o
 */
void _obl_write(struct obl_object *o);

/**
 * Atomically removes an object from any internal data structures.
 *
 * @param o
 *
 * @sa obl_destroy_object()
 */
void _obl_database_release(struct obl_object *o);

#endif
