/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the database, an environment object used to store data and mediate
 * access to a single ObjectLite database file.
 */

#include "database.h"
#include "constants.h"

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

  database->error = NULL;
  database->filename = filename;

  cache = obl_cache_create(DEFAULT_CACHE_BUCKETS, DEFAULT_CACHE_SIZE);
  if( cache == NULL ) {
    database->error = "Unable to allocate cache.";
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
  return database->error == NULL;
}
