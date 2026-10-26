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

static size_t expansion_end(const char *in, size_t len, size_t start) {
    char closing[128];
    char quotes[128] = {0};
    size_t depth = 1;
    closing[0] = in[start + 1] == '{' ? '}' : ')';
    for (size_t i = start + 2; i < len; i++) {
        char c = in[i];
        char quote = quotes[depth - 1];
        if (c == '\\' && quote != '\'' && i + 1 < len) {
            i++;
        } else if (quote == '\'' || quote == 'A') {
            if (c == '\'') quotes[depth - 1] = 0;
        } else if (c == '$' && i + 1 < len && (in[i + 1] == '{' || in[i + 1] == '(')) {
            if (depth == sizeof(closing)) return len;
            closing[depth] = in[++i] == '{' ? '}' : ')';
            quotes[depth++] = 0;
        } else if (c == '"') {
            quotes[depth - 1] = quote == '"' ? 0 : '"';
        } else if (quote == 0 && c == closing[depth - 1]) {
            if (--depth == 0) return i + 1;
        } else if (c == '`') {
            if (depth == sizeof(closing)) return len;
            closing[depth] = '`';
            quotes[depth++] = 0;
        } else if (quote == 0 && c == '\'') {
            quotes[depth - 1] = i > 0 && in[i - 1] == '$' ? 'A' : '\'';
        } else if (quote == 0 && c == '(' && closing[depth - 1] == ')') {
            if (depth == sizeof(closing)) return len;
            closing[depth] = ')';
            quotes[depth++] = 0;
        }
    }
    return len;
}

dc_result_t dc_strip_shell(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    (void)flags;
    sh_state_t state = SH_CODE;
    dc_result_t r;
    int is_first_line = 1;
    int word_start = 1;
    dc_boundary_t boundary;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;
    dc_boundary_init(&boundary);

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        switch (state) {

        case SH_CODE:
            if (c == '#' && word_start) {
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
                        word_start = 1;
                    }
                } else {
                    dc_boundary_start_comment(out, &boundary);
                    state = SH_COMMENT;
                }
            } else if (c == '\'') {
                state = SH_STRING_SQ;
                word_start = 0;
                dc_push_code(out, &boundary, c);
            } else if (c == '"') {
                state = SH_STRING_DQ;
                word_start = 0;
                dc_push_code(out, &boundary, c);
            } else if (c == '$' && i + 1 < len && in[i + 1] == '\'') {
                state = SH_DOLLAR_SQ;
                word_start = 0;
                dc_push_code(out, &boundary, '$');
                dc_push_code(out, &boundary, '\'');
                i++;
            } else if (c == '$' && i + 1 < len && (in[i + 1] == '{' || in[i + 1] == '(')) {
                size_t end = expansion_end(in, len, i);
                word_start = 0;
                while (i < end) dc_push_code(out, &boundary, in[i++]);
                i--;
            } else if (c == '\\' && i + 1 < len) {
                 
                dc_push_code(out, &boundary, c);
                i++;
                dc_push_code(out, &boundary, in[i]);
                if (in[i] != '\n') word_start = 0;
            } else {
                dc_push_code(out, &boundary, c);
                if (c == '\n') {
                    is_first_line = 0;
                    word_start = 1;
                } else {
                    word_start = c == ' ' || c == '\t' || c == ';' ||
                                 c == '&' || c == '|' || c == '(' ||
                                 c == ')' || c == '<' || c == '>';
                }
            }
            break;

        case SH_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
                state = SH_CODE;
                is_first_line = 0;
                word_start = 1;
            }
            break;

        case SH_STRING_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\'') state = SH_CODE;
             
            break;

        case SH_STRING_DQ:
            dc_push_code(out, &boundary, c);
            if (c == '$' && i + 1 < len && (in[i + 1] == '{' || in[i + 1] == '(')) {
                size_t end = expansion_end(in, len, i);
                while (++i < end) dc_push_code(out, &boundary, in[i]);
                i--;
            } else if (c == '\\') state = SH_STRING_DQ_ESC;
            else if (c == '"') state = SH_CODE;
            break;

        case SH_STRING_DQ_ESC:
            dc_push_code(out, &boundary, c);
            state = SH_STRING_DQ;
            break;

        case SH_DOLLAR_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = SH_DOLLAR_SQ_ESC;
            else if (c == '\'') state = SH_CODE;
            break;

        case SH_DOLLAR_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = SH_DOLLAR_SQ;
            break;
        }
    }

    dc_boundary_finish(out, &boundary);
    return DC_OK;
}
