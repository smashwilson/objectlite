/*
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * Any necessary emulated functionality.
 */

#include "platform.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * Emulate the POSIX mmap() function.
 */
void *mmap(void *start, size_t length, int prot, int flags,
        int fd, off_t offset)
{
    HANDLE handle;
    DWORD handle_protect;
    DWORD map_access;

    /* Derive file handle and mapping protections from +prot+ flags. */
    switch (prot) {
    case PROT_READ:
        handle_protect = PAGE_READONLY;
        map_access = FILE_MAP_READ;
        break;
    case PROT_WRITE:
    case PROT_READ | PROT_WRITE:
        handle_protect = PAGE_READWRITE;
        map_access = FILE_MAP_WRITE;
        break;
    default:
        /* Invalid use of mmap(). */
        return MAP_FAILED;
        break;
    }

    if (start != NULL || !(flags & MAP_PRIVATE)) {
        /* Invalid use of this very limited mmap(). */
        return MAP_FAILED;
    }

    handle = CreateFileMapping( (HANDLE) _get_osfhandle(fd),
            NULL, handle_protect, 0, 0, NULL);

    if (handle != NULL) {
        start = MapViewOfFile(handle, map_access, 0, offset, length);
        CloseHandle(handle);
    } else {
        start = MAP_FAILED;
    }

    return start;
}

/*
 * Emulates the POSIX munmap() function.
 */
int munmap(void *start, size_t length)
{
    return UnmapViewOfFile(start);
}

#endif
