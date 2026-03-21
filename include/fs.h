#ifndef DC_FS_H
#define DC_FS_H

#include <stddef.h>
#include "decomment.h"
#include "cli.h"
#include "log.h"
#include "lang.h"

dc_result_t dc_read_file(const char *path, char **out_data, size_t *out_len);
dc_result_t dc_safe_write(const char *path, const char *data, size_t len,
                          int do_backup, const char *backup_suffix);
dc_action_t dc_process_file(const char *path, const dc_cli_opts_t *opts,
                            dc_summary_t *summary);
void dc_process_path(const char *path, const dc_cli_opts_t *opts,
                     dc_summary_t *summary, int *had_error);
const char *dc_get_extension(const char *path);
const char *dc_get_filename(const char *path);
int dc_is_hidden(const char *path); 
int dc_pattern_match(const char *pattern, const char *str);

#endif  
