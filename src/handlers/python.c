#include "handlers.h"

typedef enum {
    PY_CODE,
    PY_COMMENT,
    PY_STRING_SQ,
    PY_STRING_SQ_ESC,
    PY_STRING_DQ,
    PY_STRING_DQ_ESC,
    PY_TRIPLE_SQ,
    PY_TRIPLE_SQ_ESC,
    PY_TRIPLE_DQ,
    PY_TRIPLE_DQ_ESC,
    PY_TRIPLE_SQ_Q1,    
    PY_TRIPLE_SQ_Q2,    
    PY_TRIPLE_DQ_Q1,    
    PY_TRIPLE_DQ_Q2     
} py_state_t;

 
static int is_string_prefix(char c) {
    return c == 'r' || c == 'R' || c == 'b' || c == 'B' ||
           c == 'f' || c == 'F' || c == 'u' || c == 'U';
}

dc_result_t dc_strip_python(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    (void)flags;
    py_state_t state = PY_CODE;
    dc_result_t r;
    int is_first_line = 1;
    dc_boundary_t boundary;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;
    dc_boundary_init(&boundary);

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case PY_CODE:
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
                    state = PY_COMMENT;
                }
            } else if (c == '\'') {
                 
                if (i + 2 < len && in[i + 1] == '\'' && in[i + 2] == '\'') {
                    state = PY_TRIPLE_SQ;
                    dc_push_code(out, &boundary, '\'');
                    dc_push_code(out, &boundary, '\'');
                    dc_push_code(out, &boundary, '\'');
                    i += 2;
                } else {
                    state = PY_STRING_SQ;
                    dc_push_code(out, &boundary, c);
                }
            } else if (c == '"') {
                if (i + 2 < len && in[i + 1] == '"' && in[i + 2] == '"') {
                    state = PY_TRIPLE_DQ;
                    dc_push_code(out, &boundary, '"');
                    dc_push_code(out, &boundary, '"');
                    dc_push_code(out, &boundary, '"');
                    i += 2;
                } else {
                    state = PY_STRING_DQ;
                    dc_push_code(out, &boundary, c);
                }
            } else if (is_string_prefix(c) && i + 1 < len) {
                 
                 
                size_t j = i + 1;
                 
                if (is_string_prefix(in[j]) && j + 1 < len) j++;
                if (in[j] == '\'' || in[j] == '"') {
                     
                    while (i < j) {
                        dc_push_code(out, &boundary, in[i]);
                        i++;
                    }
                     
                    c = in[i];
                    if (c == '\'') {
                        if (i + 2 < len && in[i + 1] == '\'' && in[i + 2] == '\'') {
                            state = PY_TRIPLE_SQ;
                            dc_push_code(out, &boundary, '\'');
                            dc_push_code(out, &boundary, '\'');
                            dc_push_code(out, &boundary, '\'');
                            i += 2;
                        } else {
                            state = PY_STRING_SQ;
                            dc_push_code(out, &boundary, c);
                        }
                    } else {
                        if (i + 2 < len && in[i + 1] == '"' && in[i + 2] == '"') {
                            state = PY_TRIPLE_DQ;
                            dc_push_code(out, &boundary, '"');
                            dc_push_code(out, &boundary, '"');
                            dc_push_code(out, &boundary, '"');
                            i += 2;
                        } else {
                            state = PY_STRING_DQ;
                            dc_push_code(out, &boundary, c);
                        }
                    }
                } else {
                    dc_push_code(out, &boundary, in[i]);
                }
            } else {
                dc_push_code(out, &boundary, c);
                if (c == '\n') is_first_line = 0;
            }
            break;

        case PY_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
                state = PY_CODE;
                is_first_line = 0;
            }
            break;

        case PY_STRING_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = PY_STRING_SQ_ESC;
            else if (c == '\'') state = PY_CODE;
            else if (c == '\n') state = PY_CODE;  
            break;

        case PY_STRING_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = PY_STRING_SQ;
            break;

        case PY_STRING_DQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = PY_STRING_DQ_ESC;
            else if (c == '"') state = PY_CODE;
            else if (c == '\n') state = PY_CODE;
            break;

        case PY_STRING_DQ_ESC:
            dc_push_code(out, &boundary, c);
            state = PY_STRING_DQ;
            break;

        case PY_TRIPLE_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = PY_TRIPLE_SQ_ESC;
            else if (c == '\'') state = PY_TRIPLE_SQ_Q1;
            break;

        case PY_TRIPLE_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = PY_TRIPLE_SQ;
            break;

        case PY_TRIPLE_SQ_Q1:
            dc_push_code(out, &boundary, c);
            if (c == '\'') state = PY_TRIPLE_SQ_Q2;
            else if (c == '\\') state = PY_TRIPLE_SQ_ESC;
            else state = PY_TRIPLE_SQ;
            break;

        case PY_TRIPLE_SQ_Q2:
            dc_push_code(out, &boundary, c);
            if (c == '\'') state = PY_CODE;  
            else if (c == '\\') state = PY_TRIPLE_SQ_ESC;
            else state = PY_TRIPLE_SQ;
            break;

        case PY_TRIPLE_DQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = PY_TRIPLE_DQ_ESC;
            else if (c == '"') state = PY_TRIPLE_DQ_Q1;
            break;

        case PY_TRIPLE_DQ_ESC:
            dc_push_code(out, &boundary, c);
            state = PY_TRIPLE_DQ;
            break;

        case PY_TRIPLE_DQ_Q1:
            dc_push_code(out, &boundary, c);
            if (c == '"') state = PY_TRIPLE_DQ_Q2;
            else if (c == '\\') state = PY_TRIPLE_DQ_ESC;
            else state = PY_TRIPLE_DQ;
            break;

        case PY_TRIPLE_DQ_Q2:
            dc_push_code(out, &boundary, c);
            if (c == '"') state = PY_CODE;  
            else if (c == '\\') state = PY_TRIPLE_DQ_ESC;
            else state = PY_TRIPLE_DQ;
            break;
        }
    }

    dc_boundary_finish(out, &boundary);
    return DC_OK;
}
