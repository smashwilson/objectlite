/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/addrtreepage.h"

#include "database.h"
#include "storage/object.h"

#include <stdio.h>
#include <stdlib.h>

struct obl_object *obl_create_addrtreepage(struct obl_database *d,
        obl_uint depth)
{
    struct obl_object *result;
    struct obl_addrtreepage_storage *storage;
    obl_uint i;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }
    result->shape = obl_at_address(d, OBL_ADDRTREEPAGE_SHAPE_ADDR);

    storage = (struct obl_addrtreepage_storage*)
            malloc(sizeof(struct obl_addrtreepage_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    storage->height = depth;
    for (i = 0; i < CHUNK_SIZE; i++) {
        storage->contents[i] = OBL_PHYSICAL_UNASSIGNED;
    }
    result->storage.addrtreepage_storage = storage;

    return result;
}

struct obl_object *obl_read_addrtreepage(struct obl_object *shape,
        obl_uint *source, obl_physical_address base, int depth)
{
    struct obl_object *result;
    obl_uint height;
    obl_physical_address addr;
    int i;

    height = readable_uint(source[base + 1]);
    result = obl_create_addrtreepage(shape->database, height);

    for (i = 0; i < CHUNK_SIZE; i++) {
        addr = (obl_physical_address) readable_uint(source[base + 2 + i]);
        result->storage.addrtreepage_storage->contents[i] = addr;
    }

    return result;
}

void obl_write_addrtreepage(struct obl_object *treepage, obl_uint *dest)
{
    int i;
    obl_physical_address base;
    obl_physical_address *contents;

    base = treepage->physical_address;
    dest[base + 1] =
            writable_uint(treepage->storage.addrtreepage_storage->height);

    contents = treepage->storage.addrtreepage_storage->contents;
    for (i = 0; i < CHUNK_SIZE; i++) {
        dest[base + 2 + i] = writable_uint((obl_uint) contents[i]);
    }
}

void obl_print_addrtreepage(struct obl_object *addrtreepage,
        int depth, int indent)
{
    int in, i;
    struct obl_addrtreepage_storage *storage;

    for (in = 0; in < indent; in++) { putchar(' '); }

    if (depth == 0) {
        printf("<address tree page: 0x%08lx>\n",
                (unsigned long) addrtreepage->logical_address);
        return ;
    }

    storage = addrtreepage->storage.addrtreepage_storage;

    printf("Address Map ");
    if (storage->height == (obl_uint) 0) {
        puts("Leaf");
    } else {
        puts("Branch");
    }

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("Height: %d\n", storage->height);

    for (i = 0; i < CHUNK_SIZE; i++) {
        for (in = 0; in < indent; in++) { putchar(' '); }
        printf("[%03d] 0x%08lx\n", i,
                (unsigned long) storage->contents[i]);
    }
}
