#if !defined(_WIN32) && !defined(_WIN64)
#define _POSIX_C_SOURCE 200809L
#endif

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if DC_WINDOWS
#include <windows.h>
#include <io.h>
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#endif

#if DC_WINDOWS

dc_result_t dc_lstat_type(const char *path, dc_ftype_t *out)
{
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return DC_ERR_IO;
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        *out = DC_FTYPE_SYMLINK;
    }
    else if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        *out = DC_FTYPE_DIRECTORY;
    }
    else
    {
        *out = DC_FTYPE_REGULAR;
    }
    return DC_OK;
}

dc_result_t dc_stat_type(const char *path, dc_ftype_t *out)
{
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return DC_ERR_IO;
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        *out = DC_FTYPE_DIRECTORY;
    }
    else
    {
        *out = DC_FTYPE_REGULAR;
    }
    return DC_OK;
}

#else

static dc_ftype_t mode_to_ftype(mode_t m)
{
    if (S_ISREG(m))
        return DC_FTYPE_REGULAR;
    if (S_ISDIR(m))
        return DC_FTYPE_DIRECTORY;
    if (S_ISLNK(m))
        return DC_FTYPE_SYMLINK;
    return DC_FTYPE_OTHER;
}

dc_result_t dc_lstat_type(const char *path, dc_ftype_t *out)
{
    struct stat st;
    if (lstat(path, &st) != 0)
        return DC_ERR_IO;
    *out = mode_to_ftype(st.st_mode);
    return DC_OK;
}

dc_result_t dc_stat_type(const char *path, dc_ftype_t *out)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return DC_ERR_IO;
    *out = mode_to_ftype(st.st_mode);
    return DC_OK;
}

#endif

dc_result_t dc_fsync(FILE *f)
{
    if (fflush(f) != 0)
        return DC_ERR_IO;
#if DC_WINDOWS
    int fd = _fileno(f);
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE)
        return DC_ERR_IO;
    if (!FlushFileBuffers(h))
        return DC_ERR_IO;
#else
    int fd = fileno(f);
    if (fd < 0)
        return DC_ERR_IO;
    if (fsync(fd) != 0)
    {
        

        if (errno != EINVAL)
            return DC_ERR_IO;
    }
#endif
    return DC_OK;
}

dc_result_t dc_rename(const char *src, const char *dst)
{
#if DC_WINDOWS
     
    if (!MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING))
    {
        return DC_ERR_IO;
    }
#else
    if (rename(src, dst) != 0)
        return DC_ERR_IO;
#endif
    return DC_OK;
}

dc_result_t dc_copy_permissions(const char *src, const char *dst)
{
#if DC_WINDOWS
     
    (void)src;
    (void)dst;
    return DC_OK;
#else
    struct stat st;
    if (stat(src, &st) != 0)
        return DC_ERR_IO;
    if (chmod(dst, st.st_mode) != 0)
        return DC_ERR_IO;
    return DC_OK;
#endif
}

#if DC_WINDOWS

dc_result_t dc_readdir(const char *dir, dc_dir_cb callback, void *ctx)
{
    char pattern[MAX_PATH + 3];
    int dirlen = (int)strlen(dir);
    if (dirlen + 3 > MAX_PATH)
        return DC_ERR_IO;
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return DC_ERR_IO;

    do
    {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        dc_ftype_t ft;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            ft = DC_FTYPE_SYMLINK;
        else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            ft = DC_FTYPE_DIRECTORY;
        else
            ft = DC_FTYPE_REGULAR;

        char fullpath[MAX_PATH + 1];
        snprintf(fullpath, sizeof(fullpath), "%s\\%s", dir, fd.cFileName);
        if (callback(fullpath, fd.cFileName, ft, ctx) != 0)
            break;
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return DC_OK;
}

#else  

dc_result_t dc_readdir(const char *dir, dc_dir_cb callback, void *ctx)
{
    DIR *d = opendir(dir);
    if (!d)
        return DC_ERR_IO;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

         
        size_t dirlen = strlen(dir);
        size_t namelen = strlen(ent->d_name);
        size_t pathlen = dirlen + 1 + namelen;
        char *fullpath = malloc(pathlen + 1);
        if (!fullpath)
        {
            closedir(d);
            return DC_ERR_ALLOC;
        }
        memcpy(fullpath, dir, dirlen);
        fullpath[dirlen] = '/';
        memcpy(fullpath + dirlen + 1, ent->d_name, namelen);
        fullpath[pathlen] = '\0';

        dc_ftype_t ft;
        if (dc_lstat_type(fullpath, &ft) != DC_OK)
        {
            ft = DC_FTYPE_OTHER;
        }

        int stop = callback(fullpath, ent->d_name, ft, ctx);
        free(fullpath);
        if (stop != 0)
            break;
    }
    closedir(d);
    return DC_OK;
}

#endif

int dc_is_binary(const char *data, size_t len)
{
    size_t check = len < 8192 ? len : 8192;
    for (size_t i = 0; i < check; i++)
    {
        if (data[i] == '\0')
            return 1;
    }
    return 0;
}

long long dc_file_size(const char *path)
{
#if DC_WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad))
        return -1;
    LARGE_INTEGER sz;
    sz.HighPart = (LONG)fad.nFileSizeHigh;
    sz.LowPart = fad.nFileSizeLow;
    return (long long)sz.QuadPart;
#else
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
    return (long long)st.st_size;
#endif
}

dc_result_t dc_tempfile_near(const char *path, FILE **out_fp, char **out_path)
{
    const char *sep = strrchr(path, DC_PATH_SEP);
#if DC_WINDOWS
    const char *sep2 = strrchr(path, '/');
    if (sep2 && (!sep || sep2 > sep))
        sep = sep2;
#endif
    size_t dirlen = sep ? (size_t)(sep - path + 1) : 0;
    const char *tmpl_suffix = ".dc_tmp_XXXXXX";
    size_t tmpl_len = dirlen + strlen(tmpl_suffix);
    char *tmpl = malloc(tmpl_len + 1);
    if (!tmpl)
        return DC_ERR_ALLOC;

    if (dirlen > 0)
        memcpy(tmpl, path, dirlen);
    strcpy(tmpl + dirlen, tmpl_suffix);

#if DC_WINDOWS
     
    if (_mktemp(tmpl) == NULL)
    {
        free(tmpl);
        return DC_ERR_IO;
    }
    FILE *f = fopen(tmpl, "wb");
    if (!f)
    {
        free(tmpl);
        return DC_ERR_IO;
    }
    *out_fp = f;
    *out_path = tmpl;
    return DC_OK;
#else
    int fd = mkstemp(tmpl);
    if (fd < 0)
    {
        free(tmpl);
        return DC_ERR_IO;
    }
    FILE *f = fdopen(fd, "wb");
    if (!f)
    {
        close(fd);
        unlink(tmpl);
        free(tmpl);
        return DC_ERR_IO;
    }
    *out_fp = f;
    *out_path = tmpl;
    return DC_OK;
#endif
}
