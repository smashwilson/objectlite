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

typedef struct {
  char *filename;
} obl_database;

#endif
