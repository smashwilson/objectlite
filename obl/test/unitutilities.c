/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "unitutilities.h"

void dump_memory(char *memory, int size)
{
    int i;

    puts("");
    for (i = 0; i < size; i++) {
        printf(" [%02i:0x%02hx]", i, ((unsigned short) memory[i] & 0x00ff));
        if (i % 4 == 3) { puts(""); }
    }
    if (size % 4 != 0) {
        puts("");
    }
}
