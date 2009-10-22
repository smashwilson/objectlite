/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#ifndef STORAGETYPES_H
#define STORAGETYPES_H

/**
 * Possible values for kinds of obl_object internal storage.  Each of these
 * corresponds to a function in obl_read_functions, as defined in io.c, and an
 * obl_shape_xxx struct defined elsewhere in the storage directory.
 */
enum obl_storage_type {
    OBL_SHAPE,
    OBL_SLOTTED,
    OBL_FIXED,
    OBL_CHUNK,
    OBL_ADDRTREEPAGE,
    OBL_INTEGER,
    OBL_FLOAT,
    OBL_DOUBLE,
    OBL_CHAR,
    OBL_STRING,
    OBL_BOOLEAN,
    OBL_NIL,
    OBL_STUB,
    OBL_STORAGE_TYPE_MAX = OBL_STUB
};


#endif /* STORAGETYPES_H */
