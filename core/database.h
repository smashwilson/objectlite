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

struct _obl_database;
typedef struct _obl_database obl_database;

#include "cache.h"
#include "log.h"
#include "error.h"
#include "object.h"

/* Fixed allocation.  These logical addresses will always resolve to universally
 * accessible, constant objects that do not reside in the database.
 */
typedef enum {
  /* Special constants: nil, true, and false. */
  OBL_NIL_ADDR, OBL_TRUE_ADDR, OBL_FALSE_ADDR,

  /* The base Shape objects. */
  OBL_INTEGER_SHAPE_ADDR, OBL_FLOAT_SHAPE_ADDR, OBL_DOUBLE_SHAPE_ADDR,
  OBL_CHAR_SHAPE_ADDR, OBL_STRING_SHAPE_ADDR,

  /* A useful constant that specifies the extent of fixed space. */
  OBL_FIXED_ADDR_MAX
} obl_fixed_address;

/* ObjectLite interface layer.
 */
struct _obl_database {

  /* Location of the persisted database. */
  const char *filename;

  /* Object cache to prevent unnecessary address translations
   * and support self-referential object structures.
   */
  obl_cache *cache;

  /* Fixed object space. */
  obl_object **fixed;

  /* Logging and error structures. */
  obl_log_configuration log_config;
  error last_error;
};

/* Allocate structures for a new ObjectLite database interface layer, using all
 * of the default settings.  Database objects created in this manner must be
 * destroyed by +obl_destroy_database()+.
 */
obl_database *obl_create_database(const char *filename);

/* The most basic query: return an object that lives at a known logical address.
 */
obl_object *obl_at_address(obl_database *database,
                           const obl_logical_address address);

/* Deallocate all of the resources associated with an ObjectLite database.
 */
void obl_destroy_database(obl_database *database);

/* Return TRUE if +database+ has no active error code, or FALSE if it does.  The
 * error status of a database may be reset using the +obl_clear_error()+
 * function.
 */
int obl_database_ok(const obl_database *database);

/* Unset any active error codes in +database+.
 */
void obl_clear_error(obl_database *database);

/* Set the active error code in +database+.
 */
void obl_report_error(obl_database *database, error_code code, char *message);

#endif
