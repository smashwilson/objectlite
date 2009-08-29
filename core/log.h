/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Describes a logging subsystem for ObjectLite, including macro definitions.
 */

#ifndef LOG_H
#define LOG_H

enum obl_log_level {
  DEBUG, INFO, NOTICE, WARN, ERROR, NONE
};

struct obl_log_configuration {
  char *filename;
  enum obl_log_level level;
};

void obl_log(struct obl_log_configuration *config,
         enum obl_log_level level, 
         const char *message);

#ifndef DISABLE_LOGGING

#define OBL_CONFIG_FROM(d) (d) == NULL ? NULL : &((d)->log_config)

#define OBL_DEBUG(d, message) obl_log( OBL_CONFIG_FROM(d), DEBUG, (message) )
#define OBL_INFO(d, message) obl_log( OBL_CONFIG_FROM(d), INFO, (message) )
#define OBL_NOTICE(d, message) obl_log( OBL_CONFIG_FROM(d), NOTICE, (message) )
#define OBL_WARN(d, message) obl_log( OBL_CONFIG_FROM(d), WARN, (message) )
#define OBL_ERROR(d, message) obl_log( OBL_CONFIG_FROM(d), ERROR, (message) )

#else /* DISABLE_LOGGING defined */

#define OBL_DEBUG(d, message)
#define OBL_INFO(d, message)
#define OBL_NOTICE(d, message)
#define OBL_WARN(d, message)
#define OBL_ERROR(d, message)

#endif /* DISABLE_LOGGING */

#endif
