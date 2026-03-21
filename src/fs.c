#include "fs.h"
#include "platform.h"
#include "lang.h"
#include "log.h"
#include "buf.h"
#include "decomment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dc_result_t dc_read_file(const char *path, char **out_data, size_t *out_len)
{
    long long sz = dc_file_size(path);
    if (sz < 0)
        return DC_ERR_IO;
    if (sz > (long long)DC_MAX_FILE_SIZE)
        return DC_ERR_TOOLARGE;

    FILE *f = fopen(path, "rb");
    if (!f)
        return DC_ERR_IO;

    size_t size = (size_t)sz;
    char *data = malloc(size + 1);
    if (!data)
    {
        fclose(f);
        return DC_ERR_ALLOC;
    }

    size_t nread = fread(data, 1, size, f);
    fclose(f);

    if (nread != size)
    {

        size = nread;
    }
    data[size] = '\0';
    *out_data = data;
    *out_len = size;
    return DC_OK;
}

dc_result_t dc_safe_write(const char *path, const char *data, size_t len,
                          int do_backup, const char *backup_suffix)
{
    FILE *tmpf = NULL;
    char *tmppath = NULL;
    dc_result_t r;

    r = dc_tempfile_near(path, &tmpf, &tmppath);
    if (r != DC_OK)
        return r;

    size_t written = fwrite(data, 1, len, tmpf);
    if (written != len)
    {
        fclose(tmpf);
        remove(tmppath);
        free(tmppath);
        return DC_ERR_IO;
    }

    r = dc_fsync(tmpf);
    fclose(tmpf);
    if (r != DC_OK)
    {
        remove(tmppath);
        free(tmppath);
        return DC_ERR_IO;
    }

    dc_copy_permissions(path, tmppath);

    if (do_backup && backup_suffix)
    {
        size_t pathlen = strlen(path);
        size_t suflen = strlen(backup_suffix);
        char *bakpath = malloc(pathlen + suflen + 1);
        if (!bakpath)
        {
            remove(tmppath);
            free(tmppath);
            return DC_ERR_ALLOC;
        }
        memcpy(bakpath, path, pathlen);
        memcpy(bakpath + pathlen, backup_suffix, suflen);
        bakpath[pathlen + suflen] = '\0';

        FILE *orig = fopen(path, "rb");
        FILE *bak = fopen(bakpath, "wb");
        if (orig && bak)
        {
            char buf[8192];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), orig)) > 0)
            {
                if (fwrite(buf, 1, n, bak) != n)
                {
                    fclose(orig);
                    fclose(bak);
                    remove(tmppath);
                    free(tmppath);
                    free(bakpath);
                    return DC_ERR_IO;
                }
            }
            fclose(orig);
            dc_fsync(bak);
            fclose(bak);
            dc_copy_permissions(path, bakpath);
        }
        else
        {
            if (orig)
                fclose(orig);
            if (bak)
                fclose(bak);
            remove(tmppath);
            free(tmppath);
            free(bakpath);
            return DC_ERR_IO;
        }
        free(bakpath);
    }

    r = dc_rename(tmppath, path);
    if (r != DC_OK)
    {
        remove(tmppath);
        free(tmppath);
        return DC_ERR_IO;
    }

    free(tmppath);
    return DC_OK;
}

const char *dc_get_extension(const char *path)
{
    const char *dot = NULL;
    const char *sep = path;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            sep = p + 1;
        if (*p == '.')
            dot = p;
    }
    if (!dot || dot < sep)
        return NULL;
    return dot + 1;
}

const char *dc_get_filename(const char *path)
{
    const char *name = path;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            name = p + 1;
    }
    return name;
}

int dc_is_hidden(const char *path)
{
    const char *name = dc_get_filename(path);
    return name[0] == '.';
}

int dc_pattern_match(const char *pattern, const char *str)
{
    const char *p = pattern;
    const char *s = str;
    const char *star_p = NULL;
    const char *star_s = NULL;

    while (*s)
    {
        if (*p == '*')
        {
            star_p = p++;
            star_s = s;
        }
        else if (*p == '?' || *p == *s)
        {
            p++;
            s++;
        }
        else if (star_p)
        {
            p = star_p + 1;
            s = ++star_s;
        }
        else
        {
            return 0;
        }
    }
    while (*p == '*')
        p++;
    return *p == '\0';
}

dc_action_t dc_process_file(const char *path, const dc_cli_opts_t *opts,
                            dc_summary_t *summary)
{
    summary->files_scanned++;

    if (!opts->include_hidden && dc_is_hidden(path))
    {
        dc_log_verbose("skipped (hidden): %s", path);
        summary->files_skipped++;
        return DC_ACTION_SKIPPED;
    }

    if (opts->exclude_pattern)
    {
        const char *fname = dc_get_filename(path);
        if (dc_pattern_match(opts->exclude_pattern, fname))
        {
            dc_log_verbose("skipped (excluded): %s", path);
            summary->files_skipped++;
            return DC_ACTION_SKIPPED;
        }
    }

    if (opts->extensions)
    {
        const char *ext = dc_get_extension(path);
        if (!ext || !dc_ext_in_list(ext, opts->extensions))
        {
            dc_log_verbose("skipped (extension filter): %s", path);
            summary->files_skipped++;
            return DC_ACTION_SKIPPED;
        }
    }

    dc_ftype_t ft;
    if (dc_lstat_type(path, &ft) != DC_OK)
    {
        dc_log_error("cannot stat: %s", path);
        summary->files_error++;
        return DC_ACTION_ERROR;
    }

    if (ft == DC_FTYPE_SYMLINK)
    {
        if (opts->follow_symlinks == DC_SYM_NO)
        {
            dc_log_verbose("skipped (symlink): %s", path);
            summary->files_skipped++;
            return DC_ACTION_SKIPPED;
        }
        if (dc_stat_type(path, &ft) != DC_OK)
        {
            dc_log_error("cannot resolve symlink: %s", path);
            summary->files_error++;
            return DC_ACTION_ERROR;
        }
    }

    if (ft != DC_FTYPE_REGULAR)
    {
        dc_log_verbose("skipped (not regular file): %s", path);
        summary->files_skipped++;
        return DC_ACTION_SKIPPED;
    }

    const dc_lang_t *lang = dc_lang_detect(path, opts->force_language);
    if (!lang)
    {
        dc_log_verbose("skipped (unsupported language): %s", path);
        summary->files_skipped++;
        return DC_ACTION_SKIPPED;
    }

    summary->files_eligible++;

    char *data = NULL;
    size_t len = 0;
    dc_result_t r = dc_read_file(path, &data, &len);
    if (r != DC_OK)
    {
        if (r == DC_ERR_TOOLARGE)
            dc_log_error("file too large: %s", path);
        else
            dc_log_error("cannot read: %s", path);
        summary->files_error++;
        return DC_ACTION_ERROR;
    }

    if (dc_is_binary(data, len))
    {
        dc_log_verbose("skipped (binary file): %s", path);
        free(data);
        summary->files_skipped++;
        return DC_ACTION_SKIPPED;
    }

    if (!opts->force_language && len > 2 && data[0] == '#' && data[1] == '!')
    {
        const dc_lang_t *shebang_lang = dc_lang_detect_shebang(data, len);
        if (shebang_lang)
            lang = shebang_lang;
    }

    dc_buf_t out;
    dc_buf_init(&out);
    r = lang->strip(data, len, &out, lang->flags);
    if (r != DC_OK)
    {
        dc_log_error("strip failed for %s (%s): internal error", path, lang->display_name);
        dc_buf_free(&out);
        free(data);
        summary->files_error++;
        return DC_ACTION_ERROR;
    }

    int changed = (out.len != len || memcmp(out.data, data, len) != 0);

    if (!changed)
    {
        dc_log_info("%s: %s: unchanged", path, lang->display_name);
        dc_buf_free(&out);
        free(data);
        summary->files_unchanged++;
        return DC_ACTION_UNCHANGED;
    }

    if (opts->check_mode)
    {
        dc_log_info("%s: %s: would change", path, lang->display_name);
        dc_buf_free(&out);
        free(data);
        summary->files_modified++;
        return DC_ACTION_MODIFIED;
    }

    if (opts->dry_run)
    {
        dc_log_info("%s: %s: would modify (%zu -> %zu bytes)", path, lang->display_name, len, out.len);
        dc_buf_free(&out);
        free(data);
        summary->files_modified++;
        return DC_ACTION_MODIFIED;
    }

    if (opts->use_stdout)
    {
        fwrite(out.data, 1, out.len, stdout);
        dc_buf_free(&out);
        free(data);
        summary->files_modified++;
        return DC_ACTION_MODIFIED;
    }

    r = dc_safe_write(path, out.data, out.len, opts->backup, opts->backup_suffix);
    if (r != DC_OK)
    {
        dc_log_error("failed to write: %s", path);
        dc_buf_free(&out);
        free(data);
        summary->files_error++;
        return DC_ACTION_ERROR;
    }

    if (opts->backup)
        summary->backups_created++;

    dc_log_info("%s: %s: modified (%zu -> %zu bytes)", path, lang->display_name, len, out.len);
    dc_buf_free(&out);
    free(data);
    summary->files_modified++;
    return DC_ACTION_MODIFIED;
}

typedef struct
{
    const dc_cli_opts_t *opts;
    dc_summary_t *summary;
    int *had_error;
} walk_ctx_t;

static int walk_cb(const char *path, const char *name, dc_ftype_t type, void *ctx)
{
    walk_ctx_t *wc = (walk_ctx_t *)ctx;

    if (!wc->opts->include_hidden && name[0] == '.')
        return 0;

    if (wc->opts->exclude_pattern && dc_pattern_match(wc->opts->exclude_pattern, name))
        return 0;

    if (type == DC_FTYPE_SYMLINK)
    {
        if (wc->opts->follow_symlinks == DC_SYM_NO)
            return 0;
        dc_ftype_t resolved;
        if (dc_stat_type(path, &resolved) != DC_OK)
            return 0;
        if (resolved == DC_FTYPE_DIRECTORY && wc->opts->follow_symlinks == DC_SYM_ALL)
        {
            dc_process_path(path, wc->opts, wc->summary, wc->had_error);
            return 0;
        }
        if (resolved == DC_FTYPE_REGULAR)
        {
            dc_action_t act = dc_process_file(path, wc->opts, wc->summary);
            if (act == DC_ACTION_ERROR && wc->opts->fail_fast)
            {
                *wc->had_error = 1;
                return 1;  
            }
        }
        return 0;
    }

    if (type == DC_FTYPE_DIRECTORY)
    {
        if (wc->opts->recursive)
        {
            dc_process_path(path, wc->opts, wc->summary, wc->had_error);
        }
        return 0;
    }

    if (type == DC_FTYPE_REGULAR)
    {
        dc_action_t act = dc_process_file(path, wc->opts, wc->summary);
        if (act == DC_ACTION_ERROR && wc->opts->fail_fast)
        {
            *wc->had_error = 1;
            return 1;
        }
    }

    return 0;
}

void dc_process_path(const char *path, const dc_cli_opts_t *opts,
                     dc_summary_t *summary, int *had_error)
{
    dc_ftype_t ft;
    if (dc_lstat_type(path, &ft) != DC_OK)
    {
        dc_log_error("cannot access: %s", path);
        summary->files_error++;
        *had_error = 1;
        return;
    }

    if (ft == DC_FTYPE_SYMLINK)
    {
        if (opts->follow_symlinks == DC_SYM_NO)
        {
            dc_log_verbose("skipped (symlink): %s", path);
            summary->files_skipped++;
            return;
        }
        if (dc_stat_type(path, &ft) != DC_OK)
        {
            dc_log_error("cannot resolve symlink: %s", path);
            summary->files_error++;
            *had_error = 1;
            return;
        }
    }

    if (ft == DC_FTYPE_REGULAR)
    {
        dc_action_t act = dc_process_file(path, opts, summary);
        if (act == DC_ACTION_ERROR)
            *had_error = 1;
        return;
    }

    if (ft == DC_FTYPE_DIRECTORY)
    {
        if (!opts->recursive)
        {
            dc_log_info("skipped (directory, use -r): %s", path);
            summary->files_skipped++;
            return;
        }
        walk_ctx_t wc = {opts, summary, had_error};
        dc_result_t r = dc_readdir(path, walk_cb, &wc);
        if (r != DC_OK)
        {
            dc_log_error("cannot read directory: %s", path);
            summary->files_error++;
            *had_error = 1;
        }
        return;
    }

    dc_log_verbose("skipped (special file): %s", path);
    summary->files_skipped++;
}
