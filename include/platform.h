#ifndef DC_PLATFORM_H
#define DC_PLATFORM_H

 
#if defined(_WIN32) || defined(_WIN64)
#  define DC_WINDOWS 1
#else
#  define DC_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
#  define DC_MACOS 1
#else
#  define DC_MACOS 0
#endif

#if defined(__linux__)
#  define DC_LINUX 1
#else
#  define DC_LINUX 0
#endif

#if defined(__OpenBSD__)
#  define DC_OPENBSD 1
#else
#  define DC_OPENBSD 0
#endif

#if defined(__FreeBSD__)
#  define DC_FREEBSD 1
#else
#  define DC_FREEBSD 0
#endif

#if defined(__NetBSD__)
#  define DC_NETBSD 1
#else
#  define DC_NETBSD 0
#endif

#define DC_UNIX (DC_MACOS || DC_LINUX || DC_OPENBSD || DC_FREEBSD || DC_NETBSD)

 
#if DC_WINDOWS
#  define DC_PATH_SEP '\\'
#  define DC_PATH_SEP_STR "\\"
#else
#  define DC_PATH_SEP '/'
#  define DC_PATH_SEP_STR "/"
#endif

#include <stddef.h>
#include <stdio.h>
#include "decomment.h"

 
typedef enum {
    DC_FTYPE_REGULAR,
    DC_FTYPE_DIRECTORY,
    DC_FTYPE_SYMLINK,
    DC_FTYPE_OTHER   
} dc_ftype_t;

 
dc_result_t dc_lstat_type(const char *path, dc_ftype_t *out);
dc_result_t dc_stat_type(const char *path, dc_ftype_t *out);
dc_result_t dc_fsync(FILE *f);
dc_result_t dc_rename(const char *src, const char *dst);
dc_result_t dc_copy_permissions(const char *src, const char *dst);
 
typedef int (*dc_dir_cb)(const char *path, const char *name, dc_ftype_t type, void *ctx);
dc_result_t dc_readdir(const char *dir, dc_dir_cb callback, void *ctx);
int dc_is_binary(const char *data, size_t len);
long long dc_file_size(const char *path);
dc_result_t dc_tempfile_near(const char *path, FILE **out_fp, char **out_path);

#endif  
