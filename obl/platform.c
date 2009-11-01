/**
 * Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
 * This software is licensed as described in the file COPYING in the root
 * directory of this distribution.
 *
 * @file platform.c
 *
 * Any necessary emulated functionality.
 */

#include "platform.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

#include <errno.h>

/**
 * Emulate the POSIX mmap() function.
 *
 * @param start A suggested starting address for the mapping.  Must be NULL.
 * @param length The number of bytes to map.
 * @param prot Bitmask indicating the protections applied to this mapping.  May
 *      be one of PROT_READ, PROT_WRITE, or PROT_READ | PROT_WRITE.
 * @param flags Bitmask indicating the type of mapping to create and its
 *      visibility.  Must be MAP_SHARED.
 * @param fd File descriptor of the underlying file.
 * @param offset The address within the file at which the mapping should begin.
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

    if (start != NULL || !(flags & MAP_SHARED)) {
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

/**
 * Emulates the POSIX munmap() function.  Release a mapping previously created
 * by a call to mmap().  Unlike the POSIX function, this function always
 * releases a full mapping at once, ignoring its length parameter.
 *
 * @param start A pointer to the memory returned by mmap().
 * @param length Ignored.
 * @return 0 if successful.
 */
int munmap(void *start, size_t length)
{
    return UnmapViewOfFile(start);
}

/**
 * Emulates the POSIX sem_init function.  Initializes a new semaphore object.
 *
 * @param sem [out] Storage for the created semaphore object.
 * @param pshared Ignored.
 * @param value The initial value of this semaphore.
 * @return 0 for success.
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
    *sem = CreateSemaphore(NULL, (LONG) value, UINT_MAX, NULL);
    return 0;
}

/**
 * Emulates the POSIX sem_wait function.  Blocks the current thread until the
 * semaphore object is placed in an unsignaled state.
 *
 * @param sem A semaphore object.
 * @return EAGAIN to indicate that waiting occurred.
 */
int sem_wait(sem_t *sem)
{
    WaitForSingleObject(*sem, INFINITE);

    return EAGAIN;
}

/**
 * Emulates the POSIX sem_trywait function.  Increments the semaphore if it
 * is unsignaled; returns immediately if it is signalled.
 *
 * @param sem The semaphore to wait on.
 * @return 0 if the semaphore was successfully signalled; EAGAIN if the
 *      semaphore was already signalled.
 */
int sem_trywait(sem_t *sem)
{
    DWORD result;

    result = WaitForSingleObject(*sem, 0);

    if (result == WAIT_OBJECT_0) {
        return 0;
    } else {
        return EAGAIN;
    }
}

/**
 * Emulates the POSIX sem_post function.  Decrements a semaphore that was
 * previously locked with a call to sem_wait() or sem_trywait(), releasing
 * the resource it protects for other threads.
 *
 * @param sem The semaphore to signal.
 * @return 0 if the signal is successful.  Nonzero if an error occurred.
 */
int sem_post(sem_t *sem)
{
    LONG previous;

    return ReleaseSemaphore(*sem, (LONG) 1, &previous) != 0;
}

/**
 * Emulates the POSIX sem_destroy function.  Releases all resources associated
 * with an allocated semaphore.  As a side effect, releases all threads that
 * were waiting on this semaphore.
 *
 * @param sem The semaphore to destroy.
 * @return 0.
 */
int sem_destroy(sem_t *sem)
{
    CloseHandle(*sem);
    return 0;
}

#endif
