#ifndef DC_BUF_H
#define DC_BUF_H

#include <stddef.h>
#include "decomment.h"

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} dc_buf_t;

void dc_buf_init(dc_buf_t *b);
void dc_buf_free(dc_buf_t *b);
dc_result_t dc_buf_ensure(dc_buf_t *b, size_t additional);
dc_result_t dc_buf_push(dc_buf_t *b, char c);
dc_result_t dc_buf_append(dc_buf_t *b, const char *data, size_t len);
dc_result_t dc_buf_append_str(dc_buf_t *b, const char *str);
void dc_buf_clear(dc_buf_t *b);
 
char *dc_buf_detach(dc_buf_t *b, size_t *out_len);

#endif  
