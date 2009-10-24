/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * 
 */

#include "storage/string.h"

#include "storage/object.h"
#include "database.h"

#include "unicode/ucnv.h"

#include <stdlib.h>
#include <string.h>

struct obl_object *obl_create_string(struct obl_database *d,
        const UChar *uc, obl_uint length)
{
    UChar *copied;
    size_t bytes;

    bytes = sizeof(UChar) * (size_t) length;
    copied = (UChar *) malloc(bytes);
    if( copied == NULL ) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    memcpy(copied, uc, bytes);

    return _allocate_string(d, copied, length);
}

struct obl_object *obl_create_cstring(struct obl_database *d,
        const char *c, obl_uint length)
{
    UConverter *converter;
    size_t output_length;
    UChar *output_string;
    size_t converted_length;
    UErrorCode status = U_ZERO_ERROR;

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_errorf(d, OBL_CONVERSION_ERROR,
                "Error creating Unicode converter: %s",
                u_errorName(status));
        return NULL;
    }

    output_length = (size_t) length * 2;
    output_string = (UChar *) malloc(sizeof(UChar) * output_length);
    if (output_string == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    converted_length = ucnv_toUChars(converter, output_string, output_length,
            c, length, &status);
    ucnv_close(converter);
    if (U_FAILURE(status)) {
        obl_report_errorf(d, OBL_CONVERSION_ERROR,
                "Unicode conversion failure: %s",
                u_errorName(status));
        free(output_string);
        return NULL;
    }

    return _allocate_string(d, output_string, (obl_uint) converted_length);
}

obl_uint obl_string_size(struct obl_object *string)
{
    if (obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_size requires an object with STRING storage.");
        return 0;
    }

    return string->storage.string_storage->length;
}

size_t obl_string_chars(struct obl_object *string,
        char *buffer, size_t buffer_size)
{
    struct obl_string_storage *storage;
    UConverter *converter;
    obl_uint converted_length;
    UErrorCode status = U_ZERO_ERROR;

    if (obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_chars requires an object with STRING storage.");
        return 0;
    }

    converter = ucnv_open(NULL, &status);
    if (U_FAILURE(status)) {
        obl_report_errorf(string->database, OBL_CONVERSION_ERROR,
                "Error opening Unicode converter: %s",
                u_errorName(status));
        return 0;
    }

    storage = string->storage.string_storage;

    converted_length = ucnv_fromUChars(converter, buffer, buffer_size,
            storage->contents, storage->length, &status);
    ucnv_close(converter);

    if (U_FAILURE(status)) {
        obl_report_errorf(string->database, OBL_CONVERSION_ERROR,
                "Unable to convert string from UTF-16: %s",
                u_errorName(status));
    }

    return converted_length;
}

int obl_string_cmp(struct obl_object *string_a, struct obl_object *string_b)
{
    obl_uint length;

    if (obl_storage_of(string_a) != OBL_STRING || obl_storage_of(string_b) != OBL_STRING) {
        return -1;
    }

    length = obl_string_size(string_a);
    if (obl_string_size(string_b) != length) {
        return -1;
    }

    return memcmp(
            string_a->storage.string_storage->contents,
            string_b->storage.string_storage->contents,
            length * sizeof(UChar));
}

int obl_string_ccmp(struct obl_object *string, const char *match)
{
    struct obl_object *temp;
    int result;

    if (obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_ccmp requires a STRING object.");
        return -1;
    }

    temp = obl_create_cstring(string->database, match, strlen(match));
    if (temp == NULL) {
        obl_report_error(string->database, OBL_OUT_OF_MEMORY, NULL);
        return -1;
    }
    result = obl_string_cmp(string, temp);
    obl_destroy_object(temp);

    return result;
}

size_t obl_string_value(struct obl_object *string,
        UChar *buffer, size_t buffer_size)
{
    struct obl_string_storage *storage;
    size_t count , bytes;

    if (obl_storage_of(string) != OBL_STRING) {
        obl_report_error(string->database, OBL_WRONG_STORAGE,
                "obl_string_value called with a non-STRING object.");
        return 0;
    }

    storage = string->storage.string_storage;
    count = storage->length > buffer_size ? storage->length : buffer_size;
    bytes = count * sizeof(UChar);

    memcpy(buffer, string->storage.string_storage->contents, bytes);

    return count;
}

/* Strings are stored as UTF-16BE with a one-word length prefix. */
struct obl_object *obl_read_string(struct obl_object *shape,
        obl_uint *source, obl_physical_address base, int depth)
{
    obl_uint length;
    obl_uint i;
    UChar *casted_source;
    int casted_offset;
    UChar *contents;
    struct obl_object *o;

    length = readable_uint(source[base + 1]);
    contents = (UChar*) malloc(length * sizeof(UChar));
    if (contents == NULL) {
        obl_report_error(shape->database, OBL_OUT_OF_MEMORY, NULL);
        return obl_nil(shape->database);
    }

    casted_source = (UChar *) source;
    casted_offset = (base + 2) * (sizeof(obl_uint) / sizeof(UChar));
    for (i = 0; i < length; i++) {
        contents[i] = readable_UChar(
                casted_source[casted_offset + i]);
    }

    o = obl_create_string(shape->database, contents, length);

    free(contents);
    return o;
}

void obl_write_string(struct obl_object *string, obl_uint *dest)
{
    obl_uint length;
    obl_uint i;
    UChar *casted_dest;
    obl_physical_address casted_offset;
    UChar ch;

    length = obl_string_size(string);
    dest[string->physical_address + 1] = writable_uint(length);

    casted_dest = (UChar *) dest;
    casted_offset = (string->physical_address + 2) *
            (sizeof(obl_uint) / sizeof(UChar));

    for (i = 0; i < length; i++) {
        ch = string->storage.string_storage->contents[i];
        casted_dest[casted_offset + i] = writable_UChar(ch);
    }
}

void obl_print_string(struct obl_object *string, int depth, int indent)
{
    int in;
    char *buffer;
    size_t buffer_size, converted_size;

    buffer_size = obl_string_size(string);
    buffer = (char*) malloc(sizeof(char) * buffer_size);
    converted_size = obl_string_chars(string, buffer, buffer_size);

    for (in = 0; in < indent; in++) { putchar(' '); }
    printf("%.*s", (int) converted_size, buffer);
}

struct obl_object *_allocate_string(struct obl_database *d,
        UChar *uc, obl_uint length)
{
    struct obl_object *result;
    struct obl_string_storage *storage;

    result = _obl_allocate_object(d);
    if (result == NULL) {
        return NULL;
    }

    storage = (struct obl_string_storage *)
            malloc(sizeof(struct obl_string_storage));
    if (storage == NULL) {
        obl_report_error(d, OBL_OUT_OF_MEMORY, NULL);
        free(result);
        return NULL;
    }
    result->storage.string_storage = storage;
    result->shape = obl_at_address(d, OBL_STRING_SHAPE_ADDR);

    storage->length = length;
    storage->contents = uc;

    return result;
}
