#include "handlers.h"

typedef enum {
    SH_CODE,
    SH_COMMENT,
    SH_STRING_SQ,       
    SH_STRING_DQ,       
    SH_STRING_DQ_ESC,
    SH_DOLLAR_SQ,       
    SH_DOLLAR_SQ_ESC
} sh_state_t;

dc_result_t dc_strip_shell(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    (void)flags;
    sh_state_t state = SH_CODE;
    dc_result_t r;
    int is_first_line = 1;
    int line_start = 1;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case SH_CODE:
            if (c == '#') {
                if (is_first_line && line_start && i + 1 < len && in[i + 1] == '!') {
                     
                    dc_buf_push(out, c);
                    i++;
                    while (i < len && in[i] != '\n') {
                        dc_buf_push(out, in[i]);
                        i++;
                    }
                    if (i < len) {
                        dc_buf_push(out, '\n');
                        is_first_line = 0;
                        line_start = 1;
                    }
                } else {
                    state = SH_COMMENT;
                }
            } else if (c == '\'') {
                state = SH_STRING_SQ;
                dc_buf_push(out, c);
            } else if (c == '"') {
                state = SH_STRING_DQ;
                dc_buf_push(out, c);
            } else if (c == '$' && i + 1 < len && in[i + 1] == '\'') {
                state = SH_DOLLAR_SQ;
                dc_buf_push(out, '$');
                dc_buf_push(out, '\'');
                i++;
            } else if (c == '\\' && i + 1 < len) {
                 
                dc_buf_push(out, c);
                i++;
                dc_buf_push(out, in[i]);
                if (in[i] == '\n') {
                    line_start = 1;
                } else {
                    line_start = 0;
                }
            } else {
                dc_buf_push(out, c);
                if (c == '\n') {
                    is_first_line = 0;
                    line_start = 1;
                } else if (c != ' ' && c != '\t') {
                    line_start = 0;
                }
            }
            break;

        case SH_COMMENT:
            if (c == '\n') {
                dc_buf_push(out, '\n');
                state = SH_CODE;
                is_first_line = 0;
                line_start = 1;
            }
            break;

        case SH_STRING_SQ:
            dc_buf_push(out, c);
            if (c == '\'') state = SH_CODE;
             
            break;

        case SH_STRING_DQ:
            dc_buf_push(out, c);
            if (c == '\\') state = SH_STRING_DQ_ESC;
            else if (c == '"') state = SH_CODE;
            break;

        case SH_STRING_DQ_ESC:
            dc_buf_push(out, c);
            state = SH_STRING_DQ;
            break;

        case SH_DOLLAR_SQ:
            dc_buf_push(out, c);
            if (c == '\\') state = SH_DOLLAR_SQ_ESC;
            else if (c == '\'') state = SH_CODE;
            break;

        case SH_DOLLAR_SQ_ESC:
            dc_buf_push(out, c);
            state = SH_DOLLAR_SQ;
            break;
        }
    }

    return DC_OK;
}
