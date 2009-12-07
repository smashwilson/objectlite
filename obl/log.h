/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Describes a logging subsystem for ObjectLite, including macro definitions.
 */

#ifndef LOG_H
#define LOG_H

/* defined in database.h */
struct obl_database_config;

typedef enum
{
    L_DEFAULT = 0, L_DEBUG, L_INFO, L_NOTICE, L_WARN, L_ERROR, L_NONE
} obl_log_level;

/**
 * The available error codes.  Each error code should correspond to one
 * exception class in each language's binding.
 */
typedef enum
{
    OBL_OK,                     //!< OBL_OK
    OBL_OUT_OF_MEMORY,          //!< OBL_OUT_OF_MEMORY
    OBL_UNABLE_TO_READ_FILE,    //!< OBL_UNABLE_TO_READ_FILE
    OBL_UNABLE_TO_OPEN_FILE,    //!< OBL_UNABLE_TO_OPEN_FILE
    OBL_CONVERSION_ERROR,       //!< OBL_CONVERSION_ERROR
    OBL_WRONG_STORAGE,          //!< OBL_WRONG_STORAGE
    OBL_ARGUMENT_SIZE,          //!< OBL_ARGUMENT_SIZE
    OBL_MISSING_SYSTEM_OBJECT,  //!< OBL_MISSING_SYSTEM_OBJECT
    OBL_DATABASE_NOT_OPEN,      //!< OBL_DATABASE_NOT_OPEN
    OBL_INVALID_INDEX,          //!< OBL_INVALID_INDEX
    OBL_INVALID_ADDRESS,        //!< OBL_INVALID_ADDRESS
    OBL_ALREADY_IN_TRANSACTION, //!< OBL_ALREADY_IN_TRANSACTION
} obl_error_code;

struct obl_log_configuration
{
    char *filename;
    obl_log_level level;
};

void obl_log(struct obl_database_config *config, obl_log_level level,
        const char *message);

#ifndef DISABLE_LOGGING

#define OBL_CONFIG_FROM(d) (d) == NULL ? NULL : &((d)->configuration)

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
