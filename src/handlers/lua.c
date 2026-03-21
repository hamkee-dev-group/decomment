#include "handlers.h"

typedef enum {
    LUA_CODE,
    LUA_DASH1,            
    LUA_LINE_COMMENT,     
    LUA_LONG_COMMENT,     
    LUA_STRING_SQ,
    LUA_STRING_SQ_ESC,
    LUA_STRING_DQ,
    LUA_STRING_DQ_ESC,
    LUA_LONG_STRING        
} lua_state_t;

dc_result_t dc_strip_lua(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    (void)flags;
    lua_state_t state = LUA_CODE;
    dc_result_t r;
    int long_eq_count = 0;       
    size_t block_newlines = 0;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case LUA_CODE:
            if (c == '-') {
                state = LUA_DASH1;
            } else if (c == '[') {
                 
                size_t j = i + 1;
                int eq = 0;
                while (j < len && in[j] == '=') { eq++; j++; }
                if (j < len && in[j] == '[') {
                    long_eq_count = eq;
                    state = LUA_LONG_STRING;
                     
                    dc_buf_push(out, '[');
                    for (int e = 0; e < eq; e++) dc_buf_push(out, '=');
                    dc_buf_push(out, '[');
                    i = j;  
                } else {
                    dc_buf_push(out, c);
                }
            } else if (c == '\'') {
                state = LUA_STRING_SQ;
                dc_buf_push(out, c);
            } else if (c == '"') {
                state = LUA_STRING_DQ;
                dc_buf_push(out, c);
            } else {
                dc_buf_push(out, c);
            }
            break;

        case LUA_DASH1:
            if (c == '-') {
                 
                 
                if (i + 1 < len && in[i + 1] == '[') {
                    size_t j = i + 2;
                    int eq = 0;
                    while (j < len && in[j] == '=') { eq++; j++; }
                    if (j < len && in[j] == '[') {
                        long_eq_count = eq;
                        block_newlines = 0;
                        state = LUA_LONG_COMMENT;
                        i = j;  
                        break;
                    }
                }
                state = LUA_LINE_COMMENT;
            } else {
                 
                dc_buf_push(out, '-');
                state = LUA_CODE;
                i--;  
            }
            break;

        case LUA_LINE_COMMENT:
            if (c == '\n') {
                dc_buf_push(out, '\n');
                state = LUA_CODE;
            }
            break;

        case LUA_LONG_COMMENT:
            if (c == '\n') {
                block_newlines++;
            } else if (c == ']') {
                 
                size_t j = i + 1;
                int eq = 0;
                while (j < len && in[j] == '=') { eq++; j++; }
                if (j < len && in[j] == ']' && eq == long_eq_count) {
                     
                    if (block_newlines > 0) {
                        for (size_t n = 0; n < block_newlines; n++)
                            dc_buf_push(out, '\n');
                    } else {
                        dc_buf_push(out, ' ');
                    }
                    i = j;  
                    state = LUA_CODE;
                }
            }
            break;

        case LUA_STRING_SQ:
            dc_buf_push(out, c);
            if (c == '\\') state = LUA_STRING_SQ_ESC;
            else if (c == '\'') state = LUA_CODE;
            break;

        case LUA_STRING_SQ_ESC:
            dc_buf_push(out, c);
            state = LUA_STRING_SQ;
            break;

        case LUA_STRING_DQ:
            dc_buf_push(out, c);
            if (c == '\\') state = LUA_STRING_DQ_ESC;
            else if (c == '"') state = LUA_CODE;
            break;

        case LUA_STRING_DQ_ESC:
            dc_buf_push(out, c);
            state = LUA_STRING_DQ;
            break;

        case LUA_LONG_STRING:
            dc_buf_push(out, c);
            if (c == ']') {
                size_t j = i + 1;
                int eq = 0;
                while (j < len && in[j] == '=') { eq++; j++; }
                if (j < len && in[j] == ']' && eq == long_eq_count) {
                     
                    for (int e = 0; e < eq; e++) dc_buf_push(out, '=');
                    dc_buf_push(out, ']');
                    i = j;
                    state = LUA_CODE;
                }
            }
            break;
        }
    }

     
    if (state == LUA_DASH1) {
        dc_buf_push(out, '-');
    }

    return DC_OK;
}
