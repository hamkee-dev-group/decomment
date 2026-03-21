#include "handlers.h"
#include <string.h>

typedef enum {
    ST_CODE,
    ST_SLASH,
    ST_LINE_COMMENT,
    ST_BLOCK_COMMENT,
    ST_BLOCK_STAR,
    ST_STRING_DQ,
    ST_STRING_DQ_ESC,
    ST_STRING_SQ,
    ST_STRING_SQ_ESC,
    ST_RAW_CPP_PREFIX,
    ST_RAW_CPP,
    ST_RAW_RUST,
    ST_RAW_RUST_CLOSING,
    ST_RAW_GO,
    ST_TEMPLATE_LIT,
    ST_TEMPLATE_ESC,
    ST_REGEX,
    ST_REGEX_ESC,
    ST_REGEX_CLASS,
    ST_REGEX_CLASS_ESC,
    ST_HASH_COMMENT
} clike_state_t;

static int is_regex_prefix_char(char c) {
    switch (c) {
    case '=': case '(': case '[': case '!': case '&':
    case '|': case '?': case ':': case ';': case ',':
    case '~': case '^': case '%': case '{': case '}':
    case '+': case '-': case '*': case '<': case '>':
    case '\n': case '\r': case '\0':
        return 1;
    default:
        return 0;
    }
}

static char last_significant(const char *in, size_t pos) {
    while (pos > 0) {
        pos--;
        char c = in[pos];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            return c;
    }
    return '\0';
}

dc_result_t dc_strip_clike(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    clike_state_t state = ST_CODE;
    dc_result_t r;
    int nest_depth = 0;
    size_t block_newlines = 0;

     
    char raw_cpp_delim[32];
    size_t raw_cpp_delim_len = 0;
    char raw_cpp_close[34];
    size_t raw_cpp_close_len = 0;
    size_t raw_cpp_close_match = 0;

     
    int rust_hash_count = 0;
    int rust_closing_hashes = 0;

     
    int brace_depth[64];
    int brace_top = -1;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case ST_CODE:
            if (c == '/' && (flags & (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT))) {
                state = ST_SLASH;
            } else if (c == '#' && (flags & CLIKE_HASH_COMMENT)) {
                state = ST_HASH_COMMENT;
            } else if (c == '"') {
                if ((flags & CLIKE_RAW_STRING_CPP) && out->len > 0 && out->data[out->len - 1] == 'R') {
                     
                    raw_cpp_delim_len = 0;
                    state = ST_RAW_CPP_PREFIX;
                    dc_buf_push(out, c);
                } else {
                    state = ST_STRING_DQ;
                    dc_buf_push(out, c);
                }
            } else if (c == 'r' && (flags & CLIKE_RAW_STRING_RUST) && i + 1 < len) {
                if (in[i + 1] == '"') {
                    rust_hash_count = 0;
                    dc_buf_push(out, 'r');
                    dc_buf_push(out, '"');
                    i++;
                    state = ST_RAW_RUST;
                } else if (in[i + 1] == '#') {
                    size_t j = i + 1;
                    int hc = 0;
                    while (j < len && in[j] == '#') { hc++; j++; }
                    if (j < len && in[j] == '"') {
                        rust_hash_count = hc;
                        dc_buf_push(out, 'r');
                        for (int h = 0; h < hc; h++) dc_buf_push(out, '#');
                        dc_buf_push(out, '"');
                        i = j;
                        state = ST_RAW_RUST;
                    } else {
                        dc_buf_push(out, c);
                    }
                } else {
                    dc_buf_push(out, c);
                }
            } else if (c == '\'' && (flags & (CLIKE_CHAR_LITERAL | CLIKE_SINGLE_STRING))) {
                state = ST_STRING_SQ;
                dc_buf_push(out, c);
            } else if (c == '`' && (flags & CLIKE_RAW_STRING_GO)) {
                state = ST_RAW_GO;
                dc_buf_push(out, c);
            } else if (c == '`' && (flags & CLIKE_TEMPLATE_LIT)) {
                state = ST_TEMPLATE_LIT;
                dc_buf_push(out, c);
            } else if (c == '{' && brace_top >= 0) {
                brace_depth[brace_top]++;
                dc_buf_push(out, c);
            } else if (c == '}' && brace_top >= 0 && brace_depth[brace_top] <= 0) {
                brace_top--;
                state = ST_TEMPLATE_LIT;
                dc_buf_push(out, c);
            } else if (c == '}' && brace_top >= 0) {
                brace_depth[brace_top]--;
                dc_buf_push(out, c);
            } else {
                dc_buf_push(out, c);
            }
            break;

        case ST_SLASH:
            if (c == '/' && (flags & CLIKE_LINE_COMMENT)) {
                state = ST_LINE_COMMENT;
            } else if (c == '*' && (flags & CLIKE_BLOCK_COMMENT)) {
                state = ST_BLOCK_COMMENT;
                nest_depth = 1;
                block_newlines = 0;
            } else {
                 
                if (flags & CLIKE_REGEX_JS) {
                    char prev = last_significant(in, i - 1);
                    if (is_regex_prefix_char(prev)) {
                        dc_buf_push(out, '/');
                        dc_buf_push(out, c);
                        if (c == '\\') state = ST_REGEX_ESC;
                        else if (c == '[') state = ST_REGEX_CLASS;
                        else if (c == '\n') {
                            state = ST_CODE;  
                        } else state = ST_REGEX;
                        break;
                    }
                }
                dc_buf_push(out, '/');
                state = ST_CODE;
                i--;
            }
            break;

        case ST_LINE_COMMENT:
            if (c == '\n') {
                dc_buf_push(out, '\n');
                state = ST_CODE;
            }
            break;

        case ST_BLOCK_COMMENT:
            if (c == '\n') {
                block_newlines++;
            } else if (c == '*') {
                state = ST_BLOCK_STAR;
            } else if (c == '/' && (flags & CLIKE_NESTED_BLOCK) &&
                       i + 1 < len && in[i + 1] == '*') {
                nest_depth++;
                i++;
            }
            break;

        case ST_BLOCK_STAR:
            if (c == '/') {
                if ((flags & CLIKE_NESTED_BLOCK) && nest_depth > 1) {
                    nest_depth--;
                    state = ST_BLOCK_COMMENT;
                } else {
                    if (block_newlines > 0) {
                        for (size_t n = 0; n < block_newlines; n++)
                            dc_buf_push(out, '\n');
                    } else {
                        dc_buf_push(out, ' ');
                    }
                    state = ST_CODE;
                }
            } else if (c == '*') {
                 
            } else if (c == '\n') {
                block_newlines++;
                state = ST_BLOCK_COMMENT;
            } else {
                state = ST_BLOCK_COMMENT;
            }
            break;

        case ST_STRING_DQ:
            dc_buf_push(out, c);
            if (c == '\\') state = ST_STRING_DQ_ESC;
            else if (c == '"') state = ST_CODE;
            else if (c == '\n' && (flags & CLIKE_CHAR_LITERAL)) {
                

                state = ST_CODE;
            }
            break;

        case ST_STRING_DQ_ESC:
            dc_buf_push(out, c);
            state = ST_STRING_DQ;
            break;

        case ST_STRING_SQ:
            dc_buf_push(out, c);
            if (c == '\\') state = ST_STRING_SQ_ESC;
            else if (c == '\'') state = ST_CODE;
            else if (c == '\n' && (flags & CLIKE_CHAR_LITERAL)) {
                state = ST_CODE;
            }
            break;

        case ST_STRING_SQ_ESC:
            dc_buf_push(out, c);
            state = ST_STRING_SQ;
            break;

        case ST_RAW_CPP_PREFIX:
            dc_buf_push(out, c);
            if (c == '(') {
                raw_cpp_close[0] = ')';
                memcpy(raw_cpp_close + 1, raw_cpp_delim, raw_cpp_delim_len);
                raw_cpp_close[1 + raw_cpp_delim_len] = '"';
                raw_cpp_close_len = 2 + raw_cpp_delim_len;
                raw_cpp_close_match = 0;
                state = ST_RAW_CPP;
            } else if (raw_cpp_delim_len < sizeof(raw_cpp_delim) - 1) {
                raw_cpp_delim[raw_cpp_delim_len++] = c;
            }
            break;

        case ST_RAW_CPP:
            dc_buf_push(out, c);
            if ((size_t)(unsigned char)c == (size_t)(unsigned char)raw_cpp_close[raw_cpp_close_match]) {
                raw_cpp_close_match++;
                if (raw_cpp_close_match == raw_cpp_close_len) {
                    state = ST_CODE;
                }
            } else {
                if (raw_cpp_close_match > 0) {
                    raw_cpp_close_match = 0;
                    if (c == raw_cpp_close[0]) raw_cpp_close_match = 1;
                }
            }
            break;

        case ST_RAW_RUST:
            dc_buf_push(out, c);
            if (c == '"') {
                if (rust_hash_count == 0) {
                    state = ST_CODE;
                } else {
                    rust_closing_hashes = 0;
                    state = ST_RAW_RUST_CLOSING;
                }
            }
            break;

        case ST_RAW_RUST_CLOSING:
            dc_buf_push(out, c);
            if (c == '#') {
                rust_closing_hashes++;
                if (rust_closing_hashes == rust_hash_count) {
                    state = ST_CODE;
                }
            } else if (c == '"') {
                rust_closing_hashes = 0;
            } else {
                state = ST_RAW_RUST;
            }
            break;

        case ST_RAW_GO:
            dc_buf_push(out, c);
            if (c == '`') state = ST_CODE;
            break;

        case ST_TEMPLATE_LIT:
            dc_buf_push(out, c);
            if (c == '\\') {
                state = ST_TEMPLATE_ESC;
            } else if (c == '`') {
                state = ST_CODE;
            } else if (c == '$' && i + 1 < len && in[i + 1] == '{') {
                dc_buf_push(out, '{');
                i++;
                if (brace_top < 62) {
                    brace_top++;
                    brace_depth[brace_top] = 0;
                }
                state = ST_CODE;
            }
            break;

        case ST_TEMPLATE_ESC:
            dc_buf_push(out, c);
            state = ST_TEMPLATE_LIT;
            break;

        case ST_REGEX:
            dc_buf_push(out, c);
            if (c == '\\') state = ST_REGEX_ESC;
            else if (c == '[') state = ST_REGEX_CLASS;
            else if (c == '/') {
                while (i + 1 < len && ((in[i + 1] >= 'a' && in[i + 1] <= 'z') ||
                                        (in[i + 1] >= 'A' && in[i + 1] <= 'Z'))) {
                    i++;
                    dc_buf_push(out, in[i]);
                }
                state = ST_CODE;
            } else if (c == '\n') {
                state = ST_CODE;
            }
            break;

        case ST_REGEX_ESC:
            dc_buf_push(out, c);
            state = ST_REGEX;
            break;

        case ST_REGEX_CLASS:
            dc_buf_push(out, c);
            if (c == '\\') state = ST_REGEX_CLASS_ESC;
            else if (c == ']') state = ST_REGEX;
            break;

        case ST_REGEX_CLASS_ESC:
            dc_buf_push(out, c);
            state = ST_REGEX_CLASS;
            break;

        case ST_HASH_COMMENT:
            if (c == '\n') {
                dc_buf_push(out, '\n');
                state = ST_CODE;
            }
            break;
        }
    }

     
    if (state == ST_SLASH) {
        dc_buf_push(out, '/');
    } else if (state == ST_BLOCK_COMMENT || state == ST_BLOCK_STAR) {
        for (size_t n = 0; n < block_newlines; n++)
            dc_buf_push(out, '\n');
    }

    return DC_OK;
}
