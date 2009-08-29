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

typedef struct {
  char *filename;
  obl_cache *cache;
  obl_log_configuration log_config;
  char *error;
} obl_database;

obl_database *obl_create_database(char *filename);

void obl_destroy_database(obl_database *database);

int obl_database_ok(obl_database *database);

#endif
