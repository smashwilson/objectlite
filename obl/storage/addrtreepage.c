/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/addrtreepage.h"

#include "database.h"
#include "session.h"
#include "storage/object.h"

#include <stdio.h>
#include <stdlib.h>

struct obl_object *obl_create_addrtreepage(obl_uint depth)
{
    struct obl_object *result;
    struct obl_addrtreepage_storage *storage;
    obl_uint i;

    result = _obl_allocate_object();
    if (result == NULL) {
        return NULL;
    }
    result->shape = _obl_at_fixed_address(OBL_ADDRTREEPAGE_SHAPE_ADDR);

    storage = malloc(sizeof(struct obl_addrtreepage_storage));
    if (storage == NULL) {
        free(result);
        obl_report_error(NULL, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }
    storage->height = depth;
    for (i = 0; i < CHUNK_SIZE; i++) {
        storage->contents[i] = OBL_PHYSICAL_UNASSIGNED;
    }
    result->storage.addrtreepage_storage = storage;

    return result;
}

struct obl_object *obl_addrtreepage_read(struct obl_session *session,
        struct obl_object *shape, obl_uint *source, obl_physical_address base,
        int depth)
{
    struct obl_object *result;
    obl_uint height;
    obl_physical_address addr;
    int i;

    height = readable_uint(source[base + 1]);
    result = obl_create_addrtreepage(height);

    for (i = 0; i < CHUNK_SIZE; i++) {
        addr = (obl_physical_address) readable_uint(source[base + 2 + i]);
        result->storage.addrtreepage_storage->contents[i] = addr;
    }

    return result;
}

void obl_addrtreepage_write(struct obl_object *treepage, obl_uint *dest)
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

void obl_addrtreepage_print(struct obl_object *addrtreepage,
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
