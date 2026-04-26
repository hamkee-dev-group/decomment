#include "handlers.h"

 
#define HASH_SQ_NO_ESCAPE   (1u << 16)   

typedef enum {
    HASH_CODE,
    HASH_COMMENT,
    HASH_STRING_SQ,
    HASH_STRING_SQ_ESC,
    HASH_STRING_DQ,
    HASH_STRING_DQ_ESC
} hash_state_t;

dc_result_t dc_strip_hash(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    hash_state_t state = HASH_CODE;
    dc_result_t r;
    int is_first_line = 1;
    int sq_no_escape = (flags & HASH_SQ_NO_ESCAPE) != 0;
    dc_boundary_t boundary;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;
    dc_boundary_init(&boundary);

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case HASH_CODE:
            if (c == '#') {
                 
                if (is_first_line && i == 0 && i + 1 < len && in[i + 1] == '!') {
                    dc_push_code(out, &boundary, c);
                    i++;
                    while (i < len && in[i] != '\n') {
                        dc_push_code(out, &boundary, in[i]);
                        i++;
                    }
                    if (i < len) {
                        dc_push_code(out, &boundary, '\n');
                        is_first_line = 0;
                    }
                } else {
                    dc_boundary_start_comment(out, &boundary);
                    state = HASH_COMMENT;
                }
            } else if (c == '\'') {
                state = HASH_STRING_SQ;
                dc_push_code(out, &boundary, c);
            } else if (c == '"') {
                state = HASH_STRING_DQ;
                dc_push_code(out, &boundary, c);
            } else {
                dc_push_code(out, &boundary, c);
                if (c == '\n') is_first_line = 0;
            }
            break;

        case HASH_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
                state = HASH_CODE;
                is_first_line = 0;
            }
            break;

        case HASH_STRING_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\' && !sq_no_escape) state = HASH_STRING_SQ_ESC;
            else if (c == '\'') state = HASH_CODE;
            break;

        case HASH_STRING_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = HASH_STRING_SQ;
            break;

        case HASH_STRING_DQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = HASH_STRING_DQ_ESC;
            else if (c == '"') state = HASH_CODE;
            break;

        case HASH_STRING_DQ_ESC:
            dc_push_code(out, &boundary, c);
            state = HASH_STRING_DQ;
            break;
        }
    }

    dc_boundary_finish(out, &boundary);
    return DC_OK;
}
