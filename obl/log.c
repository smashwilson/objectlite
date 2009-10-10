/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implements the logging subsystem for ObjectLite.
 */

#include "log.h"

#include <stdio.h>
#include <time.h>

static const char *level_names[] = { "DEBUG", "INFO", "NOTICE", "WARN",
        "ERROR", "NONE" };

#define TS_SIZE 24

void obl_log(struct obl_log_configuration *config, obl_log_level level,
        const char *message)
{
    FILE *log_file;
    char timestamp[TS_SIZE];
    time_t timer;
    struct tm *local;

    if (config == NULL || config->level > level) {
        return;
    }

    if (config->filename != NULL) {
        log_file = fopen(config->filename, "a+");
        if (log_file == NULL) {
            fprintf(stderr, "Unable to open the logging file <%s>.",
                    config->filename);
        }
    } else {
        log_file = stderr;
    }

    timer = time(NULL);
    local = localtime(&timer);
    if (!strftime(timestamp, TS_SIZE, "%d %b %Y|%I:%M:%S %p", local)) {
        return;
    }

    fprintf(log_file, "[%s] %6s %s\n", timestamp, level_names[level], message);

    if (config->filename != NULL) {
        fclose(log_file);
    }
}
