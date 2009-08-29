/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Describes a logging subsystem for ObjectLite, including macro definitions.
 */

#ifndef LOG_H
#define LOG_H

typedef enum {
  L_DEBUG, L_INFO, L_NOTICE, L_WARN, L_ERROR, L_NONE
} obl_log_level;

typedef struct {
  char *filename;
  obl_log_level level;
} obl_log_configuration;

void obl_log(obl_log_configuration *config,
             obl_log_level level,
             const char *message);

#ifndef DISABLE_LOGGING

#define OBL_CONFIG_FROM(d) (d) == NULL ? NULL : &((d)->log_config)

#define OBL_DEBUG(d, message) obl_log( OBL_CONFIG_FROM(d), L_DEBUG, (message) )
#define OBL_INFO(d, message) obl_log( OBL_CONFIG_FROM(d), L_INFO, (message) )
#define OBL_NOTICE(d, message) obl_log( OBL_CONFIG_FROM(d), L_NOTICE, (message) )
#define OBL_WARN(d, message) obl_log( OBL_CONFIG_FROM(d), L_WARN, (message) )
#define OBL_ERROR(d, message) obl_log( OBL_CONFIG_FROM(d), L_ERROR, (message) )

#else /* DISABLE_LOGGING defined */

#define OBL_DEBUG(d, message)
#define OBL_INFO(d, message)
#define OBL_NOTICE(d, message)
#define OBL_WARN(d, message)
#define OBL_ERROR(d, message)

#endif /* DISABLE_LOGGING */

#endif
