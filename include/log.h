#ifndef DC_LOG_H
#define DC_LOG_H

#include "decomment.h"
#include <stddef.h>

 
typedef enum {
    DC_LOG_QUIET = 0,
    DC_LOG_NORMAL,
    DC_LOG_VERBOSE
} dc_log_level_t;

 
void dc_log_set_level(dc_log_level_t level);
dc_log_level_t dc_log_get_level(void);

 
void dc_log_error(const char *fmt, ...);
void dc_log_warn(const char *fmt, ...);
void dc_log_info(const char *fmt, ...);
void dc_log_verbose(const char *fmt, ...);

 
typedef struct {
    int files_scanned;
    int files_eligible;
    int files_modified;
    int files_unchanged;
    int files_skipped;
    int files_error;
    int backups_created;
} dc_summary_t;

void dc_summary_init(dc_summary_t *s);
void dc_summary_print(const dc_summary_t *s);

#endif  
