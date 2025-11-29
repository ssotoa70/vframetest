/*
 * This file is part of tframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef __linux__
/* For O_DIRECT */
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/* Define O_DIRECT for platforms that don't have it */
#if !defined(O_DIRECT) && !defined(__APPLE__)
#define O_DIRECT 0
#endif

#ifdef __APPLE__
/* macOS doesn't have O_DIRECT, but we can use fcntl F_NOCACHE */
#define O_DIRECT 0
#endif

int generic_resolve_flags(platform_open_flags_t flags)
{
	int oflags = 0;

	if ((flags & (PLATFORM_OPEN_READ | PLATFORM_OPEN_WRITE)) ==
	    (PLATFORM_OPEN_READ | PLATFORM_OPEN_WRITE))
		oflags |= O_RDWR;
	else if (flags & (PLATFORM_OPEN_WRITE))
		oflags |= O_WRONLY;
	else if (flags & (PLATFORM_OPEN_READ))
		oflags |= O_RDONLY;

	if (flags & (PLATFORM_OPEN_CREATE))
		oflags |= O_CREAT;
	if (flags & (PLATFORM_OPEN_TRUNC))
		oflags |= O_TRUNC;
	if (flags & (PLATFORM_OPEN_DIRECT))
		oflags |= O_DIRECT;

	return oflags;
}

#if defined(_WIN32)
static inline platform_handle_t win_open(const char *fname,
					 platform_open_flags_t flags, int mode)
{
	unsigned int access = 0;
	unsigned int creat = 0;
	unsigned int oflags = 0;
	HANDLE h;

	if (flags & PLATFORM_OPEN_WRITE)
		access |= GENERIC_WRITE;
	if (flags & PLATFORM_OPEN_READ)
		access |= GENERIC_READ;

	if (flags & PLATFORM_OPEN_CREATE)
		creat = CREATE_ALWAYS;
	else
		creat = OPEN_EXISTING;

	if (flags & (PLATFORM_OPEN_DIRECT)) {
		oflags |= FILE_FLAG_NO_BUFFERING;
		oflags |= FILE_FLAG_WRITE_THROUGH;
	}

	h = CreateFile(fname, access, 0, NULL, creat, oflags, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	return _open_osfhandle((size_t)h, 0);
}

static inline int win_close(platform_handle_t handle)
{
	return _close(handle);
}

static inline size_t win_write(platform_handle_t handle, const char *buf,
			       size_t size)
{
	return write(handle, buf, size);
}

static inline size_t win_read(platform_handle_t handle, char *buf, size_t size)
{
	return read(handle, buf, size);
}

static inline platform_off_t win_seek(platform_handle_t handle,
				      platform_off_t offs,
				      platform_seek_flags_t whence)
{
	int posix_whence;

	switch (whence) {
	case PLATFORM_SEEK_SET:
		posix_whence = SEEK_SET;
		break;
	case PLATFORM_SEEK_CUR:
		posix_whence = SEEK_CUR;
		break;
	case PLATFORM_SEEK_END:
		posix_whence = SEEK_END;
		break;
	default:
		return -1;
	}

	return lseek(handle, offs, posix_whence);
}

int win_aligned_alloc(void **res, size_t align, size_t size)
{
	void *tmp;

	tmp = _aligned_malloc(size, align);
	if (!tmp)
		return 1;
	*res = tmp;

	return 0;
}

static inline int win_usleep(uint64_t us)
{
	return usleep((useconds_t)us);
}

static inline int win_stat(const char *fname, platform_stat_t *st)
{
	struct stat sb;
	int res;

	res = stat(fname, &sb);
	if (res)
		return res;
	if (!st)
		return res;

	st->dev = sb.st_dev;
	st->rdev = sb.st_rdev;
	st->ino = sb.st_ino;
	st->mode = sb.st_mode;
	st->uid = sb.st_uid;
	st->gid = sb.st_gid;
	st->size = sb.st_size;

	return res;
}

int win_thread_create(uint64_t *thread_id, void *(*start)(void *), void *arg)
{
	return pthread_create((pthread_t *)thread_id, NULL, start, arg);
}

int win_thread_cancel(uint64_t thread_id)
{
	return pthread_cancel((pthread_t)thread_id);
}

int win_thread_join(uint64_t thread_id, void **retval)
{
	return pthread_join((pthread_t)thread_id, retval);
}
#else

#ifdef __APPLE__
/* macOS-specific open with F_NOCACHE support for direct I/O */
static inline platform_handle_t
macos_open(const char *fname, platform_open_flags_t flags, int mode)
{
	int oflags = generic_resolve_flags(flags);
	platform_handle_t fd = open(fname, oflags, mode);

	if (fd >= 0 && (flags & PLATFORM_OPEN_DIRECT)) {
		/* Enable F_NOCACHE for direct I/O on macOS */
		if (fcntl(fd, F_NOCACHE, 1) == -1) {
			close(fd);
			return -1;
		}
		/* Also enable F_FULLFSYNC to ensure data reaches physical disk */
		fcntl(fd, F_FULLFSYNC, 1);
	}

	return fd;
}
#else
static inline platform_handle_t
generic_open(const char *fname, platform_open_flags_t flags, int mode)
{
	int oflags = generic_resolve_flags(flags);

	return open(fname, oflags, mode);
}
#endif

static inline int generic_close(platform_handle_t handle)
{
	return close(handle);
}

static inline size_t generic_write(platform_handle_t handle, const char *buf,
				   size_t size)
{
	return write(handle, buf, size);
}

static inline size_t generic_read(platform_handle_t handle, char *buf,
				  size_t size)
{
	return read(handle, buf, size);
}

static inline platform_off_t generic_seek(platform_handle_t handle,
					  platform_off_t offs,
					  platform_seek_flags_t whence)
{
	int posix_whence;

	switch (whence) {
	case PLATFORM_SEEK_SET:
		posix_whence = SEEK_SET;
		break;
	case PLATFORM_SEEK_CUR:
		posix_whence = SEEK_CUR;
		break;
	case PLATFORM_SEEK_END:
		posix_whence = SEEK_END;
		break;
	default:
		return -1;
	}

	return lseek(handle, offs, posix_whence);
}

static inline int generic_usleep(uint64_t us)
{
	return usleep((useconds_t)us);
}

#ifdef __APPLE__
/* macOS posix_memalign wrapper with proper error handling */
static inline int macos_aligned_alloc(void **res, size_t align, size_t size)
{
	int ret = posix_memalign(res, align, size);
	/* posix_memalign returns error code directly, not via errno */
	if (ret != 0) {
		*res = NULL;
		return ret;
	}
	return 0;
}
#else
/* Generic POSIX aligned allocation wrapper */
static inline int generic_aligned_alloc(void **res, size_t align, size_t size)
{
	int ret = posix_memalign(res, align, size);
	if (ret != 0) {
		*res = NULL;
		return ret;
	}
	return 0;
}
#endif

static inline int generic_stat(const char *fname, platform_stat_t *st)
{
	struct stat sb;
	int res;

	res = stat(fname, &sb);
	if (res)
		return res;
	if (!st)
		return res;

	st->dev = sb.st_dev;
	st->rdev = sb.st_rdev;
	st->ino = sb.st_ino;
	st->mode = sb.st_mode;
	st->uid = sb.st_uid;
	st->gid = sb.st_gid;
	st->size = sb.st_size;
	st->blksize = sb.st_blksize;
	st->blocks = sb.st_blocks;

	return res;
}

int generic_thread_create(uint64_t *thread_id, void *(*start)(void *),
			  void *arg)
{
	return pthread_create((pthread_t *)thread_id, NULL, start, arg);
}

int generic_thread_cancel(uint64_t thread_id)
{
	return pthread_cancel((pthread_t)thread_id);
}

int generic_thread_join(uint64_t thread_id, void **retval)
{
	return pthread_join((pthread_t)thread_id, retval);
}

#endif

static platform_t default_platform = {
#if defined(_WIN32)
	.open = win_open,
	.close = win_close,
	.write = win_write,
	.read = win_read,
	.seek = win_seek,
	.usleep = win_usleep,
	.stat = win_stat,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = win_aligned_alloc,
	.free = free,

	.thread_create = win_thread_create,
	.thread_cancel = win_thread_cancel,
	.thread_join = win_thread_join,
#elif defined(__APPLE__)
	.open = macos_open,
	.close = generic_close,
	.write = generic_write,
	.read = generic_read,
	.seek = generic_seek,
	.usleep = generic_usleep,
	.stat = generic_stat,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = macos_aligned_alloc,
	.free = free,

	.thread_create = generic_thread_create,
	.thread_cancel = generic_thread_cancel,
	.thread_join = generic_thread_join,
#else
	.open = generic_open,
	.close = generic_close,
	.write = generic_write,
	.read = generic_read,
	.seek = generic_seek,
	.usleep = generic_usleep,
	.stat = generic_stat,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = generic_aligned_alloc,
	.free = free,

	.thread_create = generic_thread_create,
	.thread_cancel = generic_thread_cancel,
	.thread_join = generic_thread_join,
#endif
};

const platform_t *platform_get(void)
{
	return &default_platform;
}

/* Phase 1: Filesystem detection and error handling implementation */

#if defined(__APPLE__)
#include <sys/mount.h>
#include <string.h>
#elif defined(__linux__)
#include <sys/statfs.h>
#include <string.h>
#endif

/* Filesystem type detection using statfs
 * Returns: 0=LOCAL, 1=SMB, 2=NFS, 3=OTHER
 */
int platform_detect_filesystem(const char *path)
{
#if defined(__APPLE__)
	/* macOS statfs mount types */
	struct statfs buf;
	if (statfs(path, &buf) != 0)
		return 3; /* OTHER on error */

	/* Check f_type (macOS uses f_type as mount type flags) */
	/* SMB magic: 0x4D4F4E4F */
	if (strcmp(buf.f_fstypename, "smbfs") == 0)
		return 1; /* SMB */
	if (strcmp(buf.f_fstypename, "nfs") == 0)
		return 2; /* NFS */

	return 0; /* LOCAL */

#elif defined(__linux__)
	/* Linux statfs filesystem types */
	struct statfs buf;
	if (statfs(path, &buf) != 0)
		return 3; /* OTHER on error */

	/* Check f_type magic numbers */
	/* NFS_SUPER_MAGIC = 0x6969 */
	if (buf.f_type == 0x6969)
		return 2; /* NFS */

	/* SMB_SUPER_MAGIC = 0x517B */
	if (buf.f_type == 0x517B)
		return 1; /* SMB */

	/* CIFS_MAGIC_NUMBER = 0xFF534D42 */
	if (buf.f_type == 0xFF534D42)
		return 1; /* SMB/CIFS */

	return 0; /* LOCAL */

#elif defined(_WIN32)
	/* Windows filesystem detection */
	char drive[4];
	char fs_name[256];

	if (path == NULL || path[0] == '\0')
		return 3; /* OTHER */

	/* Extract drive letter (e.g., "C:" from "C:\path") */
	if (path[1] == ':') {
		drive[0] = path[0];
		drive[1] = ':';
		drive[2] = '\\';
		drive[3] = '\0';
	} else if (path[0] == '\\' && path[1] == '\\') {
		/* UNC path (\\server\share) - likely SMB */
		return 1; /* SMB */
	} else {
		return 3; /* OTHER */
	}

	if (GetVolumeInformation(drive, NULL, 0, NULL, NULL, NULL,
	                        fs_name, sizeof(fs_name))) {
		if (strcmp(fs_name, "NTFS") == 0 || strcmp(fs_name, "FAT32") == 0)
			return 0; /* LOCAL */
		/* Network filesystems typically have different names */
		if (strcmp(fs_name, "SMB") == 0)
			return 1; /* SMB */
	}
	return 3; /* OTHER */

#else
	/* Unknown platform - assume local */
	return 0; /* LOCAL */
#endif
}

/* Check if direct I/O is available on file handle
 * Returns: 1 if direct I/O is available/enabled, 0 otherwise
 */
int platform_has_direct_io(platform_handle_t fd)
{
#if defined(__APPLE__)
	/* Test if F_NOCACHE is available on this file */
	int ret = fcntl(fd, F_NOCACHE, 1);
	if (ret < 0)
		return 0; /* Direct I/O not available */
	/* Restore original state */
	fcntl(fd, F_NOCACHE, 0);
	return 1;

#elif defined(__linux__)
	/* O_DIRECT is available if the file was opened with it */
	/* We can't reliably detect this post-open, so assume yes */
	return 1;

#elif defined(_WIN32)
	/* FILE_FLAG_NO_BUFFERING is generally available on Windows */
	/* Return 1 assuming it's available */
	return 1;

#else
	/* Unknown platform */
	return 0;
#endif
}

/* Get error string from errno value */
const char* platform_strerror(int error_code)
{
	return strerror(error_code);
}

/* Phase 3: Get recommended timeout for network filesystems
 * NFS: 30 second timeout (default)
 * SMB: 30 second timeout (default)
 * Local: No timeout (return 0)
 */
uint64_t platform_get_network_timeout(int filesystem_type)
{
	/* Timeout in nanoseconds: 30 seconds */
	const uint64_t NETWORK_TIMEOUT_NS = 30000000000UL;

	switch (filesystem_type) {
	case 1: /* SMB */
	case 2: /* NFS */
		return NETWORK_TIMEOUT_NS;
	case 0: /* LOCAL */
	case 3: /* OTHER */
	default:
		return 0; /* No timeout for local filesystems */
	}
}
