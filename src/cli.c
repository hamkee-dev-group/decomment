#include "cli.h"
#include "decomment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dc_cli_defaults(dc_cli_opts_t *opts) {
    memset(opts, 0, sizeof(*opts));
    opts->backup_suffix = ".bak";
    opts->follow_symlinks = DC_SYM_NO;
}

void dc_cli_version(void) {
    printf("decomment %s\n", DC_VERSION_STRING);
}

void dc_cli_help(const char *progname) {
    printf("Usage: %s [OPTIONS] [PATH...]\n", progname ? progname : "decomment");
    printf("\nSafely strip comments from source files.\n");
    printf("\nOptions:\n");
    printf("  -r, --recursive           Recurse into directories\n");
    printf("  -b, --backup              Create backup before modifying\n");
    printf("      --backup-suffix=SUF   Backup suffix (default: .bak)\n");
    printf("  -n, --dry-run             Show what would be done without writing\n");
    printf("  -v, --verbose             Verbose output\n");
    printf("  -q, --quiet               Suppress non-error output\n");
    printf("      --summary             Print summary after processing\n");
    printf("      --fail-fast           Stop on first error\n");
    printf("      --force-language=LANG Force language detection\n");
    printf("      --extensions=LIST     Only process these extensions (comma-separated)\n");
    printf("      --exclude=PATTERN     Exclude files matching pattern\n");
    printf("      --include-hidden      Process hidden files (dotfiles)\n");
    printf("      --follow-symlinks=POL Symlink policy: no, files, all (default: no)\n");
    printf("      --stdin               Read from stdin\n");
    printf("      --stdout              Write result to stdout\n");
    printf("      --check               Exit non-zero if any file would change\n");
    printf("      --no-write            Alias for --dry-run\n");
    printf("      --list-languages      List all supported languages\n");
    printf("      --list-supported      Alias for --list-languages\n");
    printf("      --version             Print version\n");
    printf("  -h, --help                Print this help\n");
    printf("\nExit codes:\n");
    printf("  0  Success\n");
    printf("  1  Operational error\n");
    printf("  2  Some files skipped or unsupported\n");
    printf("  3  Changes needed (--check mode)\n");
    printf("  4  Invalid usage\n");
    printf("\nExamples:\n");
    printf("  %s src/main.c                Strip comments from a single file\n", progname);
    printf("  %s -r src/                   Recursively process a directory\n", progname);
    printf("  %s -n -r .                   Dry-run on entire tree\n", progname);
    printf("  %s -b --backup-suffix=.orig src/main.c\n", progname);
    printf("                                  Backup original before modifying\n");
    printf("  %s --check src/              Check if files would change\n", progname);
    printf("  %s --stdin --stdout --force-language=c < input.c > output.c\n", progname);
    printf("                                  Filter mode via stdin/stdout\n");
}

 
static int match_long(const char *arg, const char *name, const char **value) {
    size_t nlen = strlen(name);
    if (strncmp(arg, name, nlen) != 0) return 0;
    if (arg[nlen] == '\0') {
        if (value) *value = NULL;
        return 1;
    }
    if (arg[nlen] == '=') {
        if (value) *value = arg + nlen + 1;
        return 1;
    }
    return 0;
}

 
static const char *get_value(const char *arg, int *i, int argc, char **argv, const char *opt_name) {
    size_t nlen = strlen(opt_name);
    if (arg[nlen] == '=') {
        return arg + nlen + 1;
    }
     
    if (*i + 1 < argc) {
        (*i)++;
        return argv[*i];
    }
    fprintf(stderr, "decomment: error: option '%s' requires a value\n", opt_name);
    return NULL;
}

int dc_cli_parse(dc_cli_opts_t *opts, int argc, char **argv) {
    dc_cli_defaults(opts);

     
    opts->paths = malloc(sizeof(char *) * (size_t)argc);
    if (!opts->paths) {
        fprintf(stderr, "decomment: error: allocation failed\n");
        return 1;
    }
    opts->path_count = 0;

    int paths_only = 0;  

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (paths_only || arg[0] != '-') {
            opts->paths[opts->path_count++] = arg;
            continue;
        }

        if (strcmp(arg, "--") == 0) {
            paths_only = 1;
            continue;
        }

         
        if (arg[0] == '-' && arg[1] == '-') {
            const char *larg = arg + 2;
            const char *val;

            if (strcmp(larg, "recursive") == 0) {
                opts->recursive = true;
            } else if (strcmp(larg, "backup") == 0) {
                opts->backup = true;
            } else if (match_long(larg, "backup-suffix", &val)) {
                opts->backup = true;
                val = get_value(larg, &i, argc, argv, "backup-suffix");
                if (!val) return 1;
                opts->backup_suffix = val;
            } else if (strcmp(larg, "dry-run") == 0) {
                opts->dry_run = true;
            } else if (strcmp(larg, "no-write") == 0) {
                opts->dry_run = true;
            } else if (strcmp(larg, "verbose") == 0) {
                opts->verbose = true;
            } else if (strcmp(larg, "quiet") == 0) {
                opts->quiet = true;
            } else if (strcmp(larg, "summary") == 0) {
                opts->summary = true;
            } else if (strcmp(larg, "fail-fast") == 0) {
                opts->fail_fast = true;
            } else if (match_long(larg, "force-language", &val)) {
                val = get_value(larg, &i, argc, argv, "force-language");
                if (!val) return 1;
                opts->force_language = val;
            } else if (match_long(larg, "extensions", &val)) {
                val = get_value(larg, &i, argc, argv, "extensions");
                if (!val) return 1;
                opts->extensions = val;
            } else if (match_long(larg, "exclude", &val)) {
                val = get_value(larg, &i, argc, argv, "exclude");
                if (!val) return 1;
                opts->exclude_pattern = val;
            } else if (strcmp(larg, "include-hidden") == 0) {
                opts->include_hidden = true;
            } else if (match_long(larg, "follow-symlinks", &val)) {
                val = get_value(larg, &i, argc, argv, "follow-symlinks");
                if (!val) return 1;
                if (strcmp(val, "no") == 0) opts->follow_symlinks = DC_SYM_NO;
                else if (strcmp(val, "files") == 0) opts->follow_symlinks = DC_SYM_FILES;
                else if (strcmp(val, "all") == 0) opts->follow_symlinks = DC_SYM_ALL;
                else {
                    fprintf(stderr, "decomment: error: invalid symlink policy '%s' (expected: no, files, all)\n", val);
                    return 1;
                }
            } else if (strcmp(larg, "stdin") == 0) {
                opts->use_stdin = true;
            } else if (strcmp(larg, "stdout") == 0) {
                opts->use_stdout = true;
            } else if (strcmp(larg, "check") == 0) {
                opts->check_mode = true;
            } else if (strcmp(larg, "list-languages") == 0) {
                opts->list_languages = true;
            } else if (strcmp(larg, "list-supported") == 0) {
                opts->list_languages = true;
            } else if (strcmp(larg, "version") == 0) {
                opts->show_version = true;
            } else if (strcmp(larg, "help") == 0) {
                opts->show_help = true;
            } else {
                fprintf(stderr, "decomment: error: unknown option '--%s'\n", larg);
                return 1;
            }
            continue;
        }

         
        for (const char *p = arg + 1; *p; p++) {
            switch (*p) {
            case 'r': opts->recursive = true; break;
            case 'b': opts->backup = true; break;
            case 'n': opts->dry_run = true; break;
            case 'v': opts->verbose = true; break;
            case 'q': opts->quiet = true; break;
            case 'h': opts->show_help = true; break;
            default:
                fprintf(stderr, "decomment: error: unknown option '-%c'\n", *p);
                return 1;
            }
        }
    }

    return 0;
}
