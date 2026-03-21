#ifndef DC_CLI_H
#define DC_CLI_H

#include <stdbool.h>

 
typedef enum {
    DC_SYM_NO = 0,
    DC_SYM_FILES,
    DC_SYM_ALL
} dc_sym_policy_t;

 
typedef struct {
     
    const char **paths;
    int path_count;

     
    bool recursive;
    bool dry_run;
    bool check_mode;
    bool verbose;
    bool quiet;
    bool summary;
    bool fail_fast;
    bool include_hidden;
    bool use_stdin;
    bool use_stdout;
    bool no_write;
    bool list_languages;
    bool list_supported;
    bool show_version;
    bool show_help;

     
    bool backup;
    const char *backup_suffix;

     
    const char *force_language;
    const char *extensions;
    const char *exclude_pattern;

     
    dc_sym_policy_t follow_symlinks;
} dc_cli_opts_t;

 
void dc_cli_defaults(dc_cli_opts_t *opts);



int dc_cli_parse(dc_cli_opts_t *opts, int argc, char **argv);

 
void dc_cli_help(const char *progname);

 
void dc_cli_version(void);

#endif  
