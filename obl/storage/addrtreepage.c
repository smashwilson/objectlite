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

void obl_print_addrtreepage(struct obl_object *addrtreepage,
        int depth, int indent)
{
    int in, i;
    struct obl_addrtreepage_storage *storage;

    for (in = 0; in < indent; in++) { putchar(' '); }

    if (depth == 0) {
        printf("<address tree page: 0x%08lx>\n", addrtreepage->logical_address);
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
        printf("[%03d] 0x%08lx\n", i, storage->contents[i]);
    }
}
