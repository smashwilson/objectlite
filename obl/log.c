/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Implements the logging subsystem for ObjectLite.
 */

#include "log.h"

#include "database.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const char *level_names[] = {
        "<default>", "DEBUG", "INFO", "NOTICE", "WARN", "ERROR", "NONE"
};

static enum obl_log_level ambient_log_level = L_NOTICE;

#define TS_SIZE 24

void obl_set_ambient_log_level(enum obl_log_level level)
{
    ambient_log_level = level;
}

void obl_log(struct obl_database_config *config, enum obl_log_level level,
        const char *message)
{
    const char *filename;
    FILE *log_file;
    char timestamp[TS_SIZE];
    time_t timer;
    struct tm *local;

    if (config == NULL) {
        if (ambient_log_level > level)
            return ;
        filename = NULL;
    } else {
        if (config->log_level > level)
            return ;
        filename = config->filename;
    }

    if (filename != NULL) {
        log_file = fopen(filename, "a+");
        if (log_file == NULL) {
            fprintf(stderr, "Unable to open the logging file <%s>.",
                    filename);
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

    if (filename != NULL) {
        fclose(log_file);
    }
}

void obl_logf(struct obl_database_config *config, enum obl_log_level level,
        const char *pattern, ...)
{
    va_list args;
    size_t required_size;
    char *buffer;

    va_start(args, pattern);
    required_size = vsnprintf(NULL, 0, pattern, args) + 1;
    va_end(args);

    va_start(args, pattern);
    buffer = malloc(required_size);
    vsnprintf(buffer, required_size, pattern, args);
    va_end(args);

    obl_log(config, level, buffer);

    free(buffer);
}
