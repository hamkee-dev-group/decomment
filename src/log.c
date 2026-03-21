#include "log.h"
#include <stdio.h>
#include <stdarg.h>

static dc_log_level_t g_log_level = DC_LOG_NORMAL;

void dc_log_set_level(dc_log_level_t level)
{
    g_log_level = level;
}

dc_log_level_t dc_log_get_level(void)
{
    return g_log_level;
}

void dc_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "decomment: error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void dc_log_warn(const char *fmt, ...)
{
    if (g_log_level < DC_LOG_NORMAL)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "decomment: warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void dc_log_info(const char *fmt, ...)
{
    if (g_log_level < DC_LOG_NORMAL)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void dc_log_verbose(const char *fmt, ...)
{
    if (g_log_level < DC_LOG_VERBOSE)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "  ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void dc_summary_init(dc_summary_t *s)
{
    s->files_scanned = 0;
    s->files_eligible = 0;
    s->files_modified = 0;
    s->files_unchanged = 0;
    s->files_skipped = 0;
    s->files_error = 0;
    s->backups_created = 0;
}

void dc_summary_print(const dc_summary_t *s)
{
    fprintf(stdout, "\nSummary:\n");
    fprintf(stdout, "  Files scanned:   %d\n", s->files_scanned);
    fprintf(stdout, "  Files eligible:  %d\n", s->files_eligible);
    fprintf(stdout, "  Files modified:  %d\n", s->files_modified);
    fprintf(stdout, "  Files unchanged: %d\n", s->files_unchanged);
    fprintf(stdout, "  Files skipped:   %d\n", s->files_skipped);
    fprintf(stdout, "  Errors:          %d\n", s->files_error);
    fprintf(stdout, "  Backups created: %d\n", s->backups_created);
}
