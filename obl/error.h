/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Declares the structure used to report errors.  Lists the available error
 * codes.
 */

#ifndef ERROR_H
#define ERROR_H

/*
 * The available error codes.  Each error code should correspond to one
 * exception class in each language's binding.
 */
typedef enum
{
    OBL_OK,
    OBL_OUT_OF_MEMORY,
    OBL_UNABLE_TO_READ_FILE,
    OBL_UNABLE_TO_OPEN_FILE,
    OBL_CONVERSION_ERROR
} error_code;

/*
 * A structure for storing information about an error that's occurred.
 */
typedef struct
{
    char *message;
    error_code code;
} error;

#endif
