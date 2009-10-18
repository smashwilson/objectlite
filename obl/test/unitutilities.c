/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "unitutilities.h"

#include <stdio.h>

void dump_memory(char *memory, int size, const char *filename)
{
    int i;
    FILE *outf;

    if (filename != NULL) {
        outf = fopen(filename, "w");
    } else {
        outf = stderr;
    }

    fputc('\n', outf);
    for (i = 0; i < size; i++) {
        if (i % 4 == 0) {
            fprintf(outf, "%4d", i / 4);
        }
        fprintf(outf, " [%4i:0x%02hx]", i, ((unsigned short) memory[i] & 0x00ff));
        if (i % 4 == 3) {
            fputc('\n', outf);
        }
    }
    if (size % 4 != 0) {
        fputc('\n', outf);
    }

    fclose(outf);
}
