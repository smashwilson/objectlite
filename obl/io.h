/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Primitive functions to deal with file I/O.
 */

#ifndef IO_H
#define IO_H

#include "platform.h"

/* Defined in object.h */
struct obl_object;

/* Declared in database.h */
struct obl_database;

/*
 * Signature of a function that reads and populates an object's internal state.
 * Each such function is provided the shape determined from the shape word, the
 * memory mapped to the database file, and an offset at which reading is to
 * occur.
 */
typedef struct obl_object *(*obl_object_read_function)(
        struct obl_object *shape, obl_uint *source,
        obl_physical_address offset, int depth);

/*
 * Signature of a function that writes an obl_object into a memory-mapped file,
 * not including its shape header word.  The object will be written at a
 * location specified by its assigned physical address.
 */
typedef void (*obl_object_write_function)(
        struct obl_object *object, obl_uint *source);

/* Read a single-word obl_integer_object. */
struct obl_object *obl_read_integer(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/* Read a length-prefixed UTF-16BE string object. */
struct obl_object *obl_read_string(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Read a slotted object.  The number of slots expected is determined by the
 * provided shape.
 */
struct obl_object *obl_read_slotted(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Read a fixed-length collection.
 */
struct obl_object *obl_read_fixed(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Read a shape object.  Shapes are themselves a fixed shape (sorry, no turtles
 * all the way down -- yet).
 */
struct obl_object *obl_read_shape(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Read an address tree page.  Address tree pages reference each other by
 * physical address (so that they can used during the address lookup process)
 * so tree pages don't respect the +depth+ parameter.
 */
struct obl_object *obl_read_addrtreepage(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Invoked for any storage type that is either not yet defined properly, or
 * isn't supposed to actually be stored in the database.
 */
struct obl_object *obl_invalid_read(struct obl_object *shape,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Read a shape word from the current position of the file <source>, retrieve
 * the shape object, then invoke the appropriate struct obl_object_read_function from
 * the <obl_read_functions> table to read the rest of the object.  Return the
 * populated struct obl_object structure.
 *
 * The created object lives on the heap and must be destroyed with a call to
 * obl_destroy_object() as defined in "object.h".
 */
struct obl_object *obl_read_object(struct obl_database *d,
        obl_uint *source, obl_physical_address offset, int depth);

/*
 * Write an integer object.
 */
void obl_write_integer(struct obl_object *integer, obl_uint *dest);

/*
 * Write a string object.
 */
void obl_write_string(struct obl_object *string, obl_uint *dest);

/*
 * Write a slotted object.
 */
void obl_write_slotted(struct obl_object *slotted, obl_uint *dest);

/*
 * Write a fixed-length collection.
 */
void obl_write_fixed(struct obl_object *fixed, obl_uint *dest);

/*
 * Write a shape object.
 */
void obl_write_shape(struct obl_object *string, obl_uint *dest);

/*
 * Write an address map tree page.
 */
void obl_write_addrtreepage(struct obl_object *treepage, obl_uint *dest);

/*
 * Invoked for any storage type that is either not defined yet, or isn't
 * supposed to actually be written to the database.
 */
void obl_invalid_write(struct obl_object *o, obl_uint *dest);

/*
 * Write an object +o+ to the memory-mapped file pointed to by +dest+.  +o+ must
 * already have a physical and logical address assigned to it, although
 * recursively referenced objects may not.
 */
void obl_write_object(struct obl_object *o, obl_uint *dest);

#endif
