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

/*
 * External functions definitions.
 */

obl_database *obl_create_database(char *filename)
{
  obl_database *database;
  obl_cache *cache;

  database = (obl_database*) malloc(sizeof(obl_database));
  if( database == NULL ) {
    return NULL;
  }

  database->filename = filename;
  obl_clear_error(database);

  cache = obl_cache_create(DEFAULT_CACHE_BUCKETS, DEFAULT_CACHE_SIZE);
  if( cache == NULL ) {
    obl_report_error(database, OUT_OF_MEMORY, "Unable to allocate cache.");
  }
  database->cache = cache;

  return database;
}

void obl_destroy_database(obl_database *database)
{
  if( database->cache != NULL ) {
    obl_cache_destroy(database->cache);
  }

  free(database);
}

int obl_database_ok(obl_database *database)
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

  if( database == NULL ) {
    fprintf(stderr, "Unable to report error \"%s\":\n", message);
    fprintf(stderr, "No database structure available to report it in.\n");
    return ;
  }

  database->last_error.message = message;
  database->last_error.code = code;
}
