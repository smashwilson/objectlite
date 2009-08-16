/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Defines the in-memory representations of objects that can be stored in ObjectLite.
 */

#include <stdlib.h>

#include "object.h"

void obl_destroy_object(obl_object *object)
{
  /* All possible internal storage slots occupy the same address, so
   * we only need to free one of them.
   */
  free(object->internal_storage.slotted);

  free(object);
}
