/*
 * Copyright(c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SGX_SAFE_FILE_OPS_H_
#define _SGX_SAFE_FILE_OPS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#if defined(__has_include)
#  if __has_include(<linux/openat2.h>)
#    include <linux/openat2.h>
#    define SGX_HAVE_OPENAT2_HEADER 1
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a file rejecting symlinks anywhere in the path.
 *
 * Uses openat2(RESOLVE_NO_SYMLINKS) when available (Linux >= 5.6) so that
 * symlinks in *any* path component (not just the final one) cause the
 * call to fail with ELOOP. Falls back to plain open(O_NOFOLLOW) on older
 * kernels (or when seccomp blocks openat2), preserving the previous
 * end-component-only behavior with no regression.
 *
 * Returns a file descriptor on success, -1 with errno set on failure.
 */
static inline int sgx_safe_open_impl(const char *path, int flags, mode_t mode)
{
#if defined(SYS_openat2)
    /* Define struct open_how locally if the kernel header is unavailable
     * (e.g. building against a glibc/headers older than 5.6). The layout
     * is part of the stable kernel ABI. */
#if !defined(SGX_HAVE_OPENAT2_HEADER)
    struct open_how {
        uint64_t flags;
        uint64_t mode;
        uint64_t resolve;
    };
#  ifndef RESOLVE_NO_SYMLINKS
#    define RESOLVE_NO_SYMLINKS 0x04
#  endif
#endif
    struct open_how how;
    memset(&how, 0, sizeof(how));
    how.flags = (uint64_t)flags;
    how.mode = (flags & (O_CREAT | O_TMPFILE)) ? (uint64_t)mode : 0;
    how.resolve = RESOLVE_NO_SYMLINKS;

    int fd = (int)syscall(SYS_openat2, AT_FDCWD, path, &how, sizeof(how));
    if (fd >= 0)
        return fd;
    /* Fall back only on lack of kernel/seccomp support for openat2.
     * ELOOP / EACCES / ENOENT etc. are real errors and must propagate. */
    if (errno != ENOSYS && errno != EPERM)
        return -1;
#endif
    /* Fallback: end-component-only protection via O_NOFOLLOW. */
    return open(path, flags | O_NOFOLLOW, mode);
}

static inline int sgx_is_symlink(const char *path)
{
    struct stat st;
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }
    int saved_errno = errno;
    if (lstat(path, &st) != 0) {
        /* Non-existence is not an error for symlink detection */
        if (errno == ENOENT || errno == ENOTDIR) {
            errno = saved_errno;
            return 0;
        }
        return -1;  /* permission/IO error - caller should fail closed */
    }
    return S_ISLNK(st.st_mode) ? 1 : 0;
}

/**
 * Symlink-safe fopen: opens a file rejecting symlinks anywhere in the path
 * via openat2(RESOLVE_NO_SYMLINKS) on Linux >= 5.6, or via O_NOFOLLOW
 * (final component only) on older kernels.
 */
static inline FILE *sgx_safe_fopen(const char *filename, const char *mode)
{
    if (filename == NULL || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Convert fopen mode string to open(2) flags.
     * Handles standard modifiers: 'b' (binary, no-op on POSIX),
     * '+' (read-write), 'x' (exclusive create), 'e' (close-on-exec). */
    int flags = O_LARGEFILE;
    int has_plus = (strchr(mode, '+') != NULL);
    if (strchr(mode, 'e'))
        flags |= O_CLOEXEC;
    if (strchr(mode, 'x'))
        flags |= O_EXCL;
    switch (mode[0]) {
        case 'r':
            flags |= has_plus ? O_RDWR : O_RDONLY;
            break;
        case 'w':
            flags |= (has_plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
            break;
        case 'a':
            flags |= (has_plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
            break;
        default:
            errno = EINVAL;
            return NULL;
    }

    int fd = sgx_safe_open_impl(filename, flags, 0666);
    if (fd < 0)
        return NULL;

    /* For append modes, position at end-of-file to match fopen() semantics.
     * O_APPEND ensures writes append, but doesn't set the initial offset. */
    if (mode[0] == 'a') {
        if (lseek(fd, 0, SEEK_END) == (off_t)-1) {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return NULL;
        }
    }

    /* Build a portable mode string for fdopen(): only r/w/a, optional b, optional +.
     * Non-portable modifiers (e, x) are already handled via open() flags above. */
    char safe_mode[4];
    int mi = 0;
    safe_mode[mi++] = mode[0];
    if (strchr(mode, 'b'))
        safe_mode[mi++] = 'b';
    if (has_plus)
        safe_mode[mi++] = '+';
    safe_mode[mi] = '\0';

    FILE *fp = fdopen(fd, safe_mode);
    if (fp == NULL) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
    }
    return fp;
}

/**
 * Symlink-safe open: rejects symlinks anywhere in the path via
 * openat2(RESOLVE_NO_SYMLINKS) on Linux >= 5.6, falling back to
 * open(O_NOFOLLOW) (final component only) on older kernels.
 */
static inline int sgx_safe_open(const char *path, int flags, mode_t mode)
{
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sgx_safe_open_impl(path, flags, mode);
}

#ifdef __cplusplus
}
#endif

#endif /* _SGX_SAFE_FILE_OPS_H_ */
