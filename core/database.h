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

#include "cache.h"
#include "log.h"
#include "error.h"

typedef struct {
  char *filename;
  obl_cache *cache;
  obl_log_configuration log_config;
  error last_error;
} obl_database;

/*
 * Allocate structures for a new ObjectLite database interface layer, using all
 * of the default settings.  Database objects created in this manner must be
 * destroyed by +obl_destroy_database()+.
 */
obl_database *obl_create_database(char *filename);

/*
 * Deallocate all of the resources associated with an ObjectLite database.
 */
void obl_destroy_database(obl_database *database);

/*
 * Return TRUE if +database+ has no active error code, or FALSE if it does.  The
 * error status of a database may be reset using the +obl_clear_error()+
 * function.
 */
int obl_database_ok(obl_database *database);

/*
 * Unset any active error codes in +database+.
 */
void obl_clear_error(obl_database *database);

/*
 * Set the active error code in +database+.
 */
void obl_report_error(obl_database *database, error_code code, char *message);

#endif
