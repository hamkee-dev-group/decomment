#include "decomment.h"
#include "cli.h"
#include "lang.h"
#include "log.h"
#include "fs.h"
#include "buf.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int handle_stdin(const dc_cli_opts_t *opts)
{
    if (!opts->force_language)
    {
        dc_log_error("--stdin requires --force-language");
        return DC_EXIT_USAGE;
    }

    const dc_lang_t *lang = dc_lang_find(opts->force_language);
    if (!lang)
    {
        dc_log_error("unknown language: %s", opts->force_language);
        return DC_EXIT_USAGE;
    }

    dc_buf_t input;
    dc_buf_init(&input);
    char buf[8192];
    size_t n;

#if DC_WINDOWS
    _setmode(_fileno(stdin), _O_BINARY);
    if (opts->use_stdout)
        _setmode(_fileno(stdout), _O_BINARY);
#endif

    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0)
    {
        if (dc_buf_append(&input, buf, n) != DC_OK)
        {
            dc_log_error("allocation failed reading stdin");
            dc_buf_free(&input);
            return DC_EXIT_ERROR;
        }
        if (input.len > DC_MAX_FILE_SIZE)
        {
            dc_log_error("stdin input exceeds maximum size (%u bytes)", DC_MAX_FILE_SIZE);
            dc_buf_free(&input);
            return DC_EXIT_ERROR;
        }
    }

    if (dc_is_binary(input.data, input.len))
    {
        dc_log_error("stdin appears to contain binary data");
        dc_buf_free(&input);
        return DC_EXIT_ERROR;
    }

    dc_buf_t output;
    dc_buf_init(&output);
    dc_result_t r = lang->strip(input.data, input.len, &output, lang->flags);
    if (r != DC_OK)
    {
        dc_log_error("strip failed");
        dc_buf_free(&input);
        dc_buf_free(&output);
        return DC_EXIT_ERROR;
    }

    if (opts->check_mode)
    {
        int changed = (output.len != input.len || memcmp(output.data, input.data, input.len) != 0);
        dc_buf_free(&input);
        dc_buf_free(&output);
        return changed ? DC_EXIT_CHECK_DIFF : DC_EXIT_SUCCESS;
    }

    fwrite(output.data, 1, output.len, stdout);
    dc_buf_free(&input);
    dc_buf_free(&output);
    return DC_EXIT_SUCCESS;
}

static void list_languages(void)
{
    const dc_lang_t *const *all = dc_lang_all();
    printf("Supported languages:\n\n");
    printf("  %-16s %-12s %s\n", "Language", "Name", "Extensions");
    printf("  %-16s %-12s %s\n", "--------", "----", "----------");
    for (int i = 0; all[i]; i++)
    {
        const dc_lang_t *l = all[i];
         
        char extbuf[256] = "";
        for (int j = 0; l->extensions[j]; j++)
        {
            if (j > 0)
                strcat(extbuf, ", ");
            strcat(extbuf, ".");
            strcat(extbuf, l->extensions[j]);
        }
        printf("  %-16s %-12s %s\n", l->display_name, l->name, extbuf);
        if (l->notes)
        {
            printf("  %-16s %-12s  Note: %s\n", "", "", l->notes);
        }
    }
}

int main(int argc, char **argv)
{
    dc_lang_init();

    dc_cli_opts_t opts;
    if (dc_cli_parse(&opts, argc, argv) != 0)
    {
        return DC_EXIT_USAGE;
    }

    if (opts.show_help)
    {
        dc_cli_help(argv[0]);
        free(opts.paths);
        return DC_EXIT_SUCCESS;
    }

    if (opts.show_version)
    {
        dc_cli_version();
        free(opts.paths);
        return DC_EXIT_SUCCESS;
    }

    if (opts.list_languages)
    {
        list_languages();
        free(opts.paths);
        return DC_EXIT_SUCCESS;
    }

    if (opts.quiet)
    {
        dc_log_set_level(DC_LOG_QUIET);
    }
    else if (opts.verbose)
    {
        dc_log_set_level(DC_LOG_VERBOSE);
    }

    if (opts.use_stdin)
    {
        int rc = handle_stdin(&opts);
        free(opts.paths);
        return rc;
    }

    if (opts.path_count == 0)
    {
        fprintf(stderr, "decomment: error: no input files (use -h for help)\n");
        free(opts.paths);
        return DC_EXIT_USAGE;
    }

    dc_summary_t summary;
    dc_summary_init(&summary);

    int had_error = 0;
    int had_check_diff = 0;

    for (int i = 0; i < opts.path_count; i++)
    {
        dc_process_path(opts.paths[i], &opts, &summary, &had_error);
        if (had_error && opts.fail_fast)
            break;
    }

    if (opts.check_mode && summary.files_modified > 0)
    {
        had_check_diff = 1;
    }

    if (opts.summary)
    {
        dc_summary_print(&summary);
    }

    free(opts.paths);

    if (had_error)
        return DC_EXIT_ERROR;
    if (had_check_diff)
        return DC_EXIT_CHECK_DIFF;
    if (summary.files_skipped > 0 && summary.files_modified == 0 && summary.files_unchanged == 0)
        return DC_EXIT_SKIPPED;
    return DC_EXIT_SUCCESS;
}
