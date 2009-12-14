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

enum obl_log_level
{
    L_DEFAULT = 0, L_DEBUG, L_INFO, L_NOTICE, L_WARN, L_ERROR, L_NONE
};

/**
 * The available error codes.  Each error code should correspond to one
 * exception class in each language's binding.
 */
enum obl_error_code
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
};

/**
 * Set the level of logging messages to be used when no database is available.
 */
void obl_set_ambient_log_level(enum obl_log_level level);

void obl_log(struct obl_database_config *config, enum obl_log_level level,
        const char *message);

void obl_logf(struct obl_database_config *config, enum obl_log_level level,
        const char *pattern, ...);

#ifndef DISABLE_LOGGING

#define OBL_CONFIG_FROM(d) (d) == NULL ? NULL : &((d)->configuration)

#define _OBL_LOG(d, level, message) \
    obl_log( OBL_CONFIG_FROM(d), (level), (message))
#define _OBL_LOGF(d, level, pattern, ...) \
    obl_logf( OBL_CONFIG_FROM(d), (level), (pattern), __VA_ARGS__)
#define _OBL_NLOG(level, message) \
    obl_log( NULL, (level), (message) )
#define _OBL_NLOGF(level, pattern, ...) \
    obl_logf( NULL, (level), (pattern), __VA_ARGS__)

#define OBL_DEBUG(d, message) \
    _OBL_LOG(d, L_DEBUG, message)
#define OBL_INFO(d, message) \
    _OBL_LOG(d, L_INFO, message)
#define OBL_NOTICE(d, message) \
    _OBL_LOG(d, L_NOTICE, message)
#define OBL_WARN(d, message) \
    _OBL_LOG(d, L_WARN, message)
#define OBL_ERROR(d, message) \
    _OBL_LOG(d, L_ERROR, message)

#define OBL_NDEBUG(message) \
    _OBL_NLOG(L_DEBUG, message)
#define OBL_NINFO(message) \
    _OBL_NLOG(L_INFO, message)
#define OBL_NNOTICE(message) \
    _OBL_NLOG(L_NOTICE, message)
#define OBL_NWARN(message) \
    _OBL_LOG(L_WARN, message)
#define OBL_NERROR(message) \
    _OBL_LOG(L_ERROR, message)

#define OBL_DEBUGF(d, pattern, ...) \
    _OBL_LOGF(d, L_DEBUG, pattern, __VA_ARGS__)
#define OBL_INFOF(d, pattern, ...) \
    _OBL_LOGF(d, L_INFO, pattern, __VA_ARGS__)
#define OBL_NOTICEF(d, pattern, ...) \
    _OBL_LOGF(d, L_NOTICE, pattern, __VA_ARGS__)
#define OBL_WARNF(d, pattern, ...) \
    _OBL_LOGF(d, L_WARN, pattern, __VA_ARGS__)
#define OBL_ERRORF(d, pattern, ...) \
    _OBL_LOGF(d, L_ERROR, pattern, __VA_ARGS__)

#define OBL_NDEBUGF(pattern, ...) \
    _OBL_NLOGF(d, L_DEBUG, pattern, __VA_ARGS__)
#define OBL_NINFOF(pattern, ...) \
    _OBL_NLOGF(d, L_INFO, pattern, __VA_ARGS__)
#define OBL_NNOTICEF(pattern, ...) \
    _OBL_NLOGF(d, L_NOTICE, pattern, __VA_ARGS__)
#define OBL_NWARNF(pattern, ...) \
    _OBL_NLOGF(d, L_WARN, pattern, __VA_ARGS__)
#define OBL_NERRORF(pattern, ...) \
    _OBL_NLOGF(d, L_ERROR, pattern, __VA_ARGS__)

#else /* DISABLE_LOGGING defined */

#define OBL_DEBUG(d, message)
#define OBL_INFO(d, message)
#define OBL_NOTICE(d, message)
#define OBL_WARN(d, message)
#define OBL_ERROR(d, message)

#define OBL_NDEBUG(message)
#define OBL_NINFO(message)
#define OBL_NNOTICE(message)
#define OBL_NWARN(message)
#define OBL_NERROR(message)

#define OBL_DEBUGF(d, pattern, ...)
#define OBL_INFOF(d, pattern, ...)
#define OBL_NOTICEF(d, pattern, ...)
#define OBL_WARNF(d, pattern, ...)
#define OBL_ERRORF(d, pattern, ...)

#define OBL_NDEBUGF(pattern, ...)
#define OBL_NINFOF(pattern, ...)
#define OBL_NNOTICEF(pattern, ...)
#define OBL_NWARNF(pattern, ...)
#define OBL_NERRORF(pattern, ...)

#endif /* DISABLE_LOGGING */

#endif
