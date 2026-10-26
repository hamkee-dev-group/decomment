#include "handlers.h"
#include <ctype.h>
#include <string.h>

typedef enum {
    ST_CODE,
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

static size_t skip_space(const char *in, size_t pos) {
    while (pos > 0 && isspace((unsigned char)in[pos - 1])) pos--;
    return pos;
}

static int is_identifier_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '$' || (unsigned char)c >= 128;
}

static int regex_keyword(const char *in, size_t end, int control) {
    static const char *const prefixes[] = {
        "return", "throw", "case", "delete", "void", "typeof", "new",
        "in", "instanceof", "yield", "await", "else", "do", NULL
    };
    static const char *const controls[] = {
        "if", "while", "for", "with", "switch", "catch", NULL
    };
    const char *const *words = control ? controls : prefixes;
    size_t start = end;
    while (start > 0 && is_identifier_char(in[start - 1])) start--;
    size_t before = skip_space(in, start);
    if (before > 0 && in[before - 1] == '.') return 0;
    for (size_t n = 0; words[n]; n++) {
        size_t word_len = strlen(words[n]);
        if (end - start == word_len && memcmp(in + start, words[n], word_len) == 0)
            return 1;
    }
    return 0;
}

static int can_start_regex(const dc_buf_t *out, int control_paren) {
    size_t end = skip_space(out->data, out->len);
    if (end == 0) return 1;
    char prev = out->data[end - 1];
    if (prev == ')') return control_paren;
    if ((prev == '+' || prev == '-') && end > 1 && out->data[end - 2] == prev)
        return 0;
    return prev == '/' || is_regex_prefix_char(prev) || regex_keyword(out->data, end, 0);
}

static size_t splice_end(const char *in, size_t len, size_t pos) {
    if (pos < len && in[pos] == '\\') {
        if (pos + 1 < len && in[pos + 1] == '\n') return pos + 2;
        if (pos + 2 < len && in[pos + 1] == '\r' && in[pos + 2] == '\n') return pos + 3;
    }
    return pos;
}

static int go_directive(const char *in, size_t len) {
    static const char *const prefixes[] = {"go:", "line ", "export ", NULL};
    for (size_t n = 0; prefixes[n]; n++) {
        size_t prefix_len = strlen(prefixes[n]);
        if (len >= prefix_len && memcmp(in, prefixes[n], prefix_len) == 0) return 1;
    }
    size_t pos = 0;
    while (pos < len && (in[pos] == ' ' || in[pos] == '\t')) pos++;
    return len - pos > 6 && memcmp(in + pos, "+build", 6) == 0 &&
           (in[pos + 6] == ' ' || in[pos + 6] == '\t');
}

static int go_imports_c(const char *in, size_t len, size_t pos) {
    if (len - pos < 6 || memcmp(in + pos, "import", 6) != 0 ||
        (pos > 0 && is_identifier_char(in[pos - 1])) ||
        (pos + 6 < len && is_identifier_char(in[pos + 6]))) return 0;
    pos += 6;
    int grouped = 0;
    while (pos < len) {
        char c = in[pos];
        if (c == '/' && pos + 1 < len && in[pos + 1] == '/') {
            while (pos < len && in[pos] != '\n') pos++;
        } else if (c == '/' && pos + 1 < len && in[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < len && !(in[pos] == '*' && in[pos + 1] == '/')) pos++;
            if (pos + 1 >= len) return 0;
            pos += 2;
        } else if (c == '"' || c == '`') {
            size_t start = ++pos;
            while (pos < len && in[pos] != c) {
                if (c == '"' && in[pos] == '\\' && pos + 1 < len) pos++;
                pos++;
            }
            if (pos < len && pos - start == 1 && in[start] == 'C') return 1;
            if (!grouped || pos == len) return 0;
            pos++;
        } else if (c == '(') {
            grouped = 1;
            pos++;
        } else if (c == ')' || c == '{' || c == '}') {
            return 0;
        } else {
            pos++;
        }
    }
    return 0;
}

dc_result_t dc_strip_clike(const char *in, size_t len, dc_buf_t *out, uint32_t flags) {
    clike_state_t state = ST_CODE;
    dc_result_t r;
    int nest_depth = 0;
    size_t block_newlines = 0;
    dc_boundary_t boundary;

     
    char raw_cpp_delim[32];
    size_t raw_cpp_delim_len = 0;
    char raw_cpp_close[34];
    size_t raw_cpp_close_len = 0;
    size_t raw_cpp_close_match = 0;

     
    int rust_hash_count = 0;
    int rust_closing_hashes = 0;

     
    int brace_depth[64];
    int brace_top = -1;
    unsigned char control_parens[256];
    size_t paren_depth = 0;
    int control_paren = 0;
    size_t output_start = out->len;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;
    dc_boundary_init(&boundary);

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        if ((flags & CLIKE_LINE_SPLICE) && state != ST_RAW_CPP && state != ST_RAW_CPP_PREFIX) {
            size_t end = splice_end(in, len, i);
            if (end != i) {
                if (state != ST_LINE_COMMENT && state != ST_BLOCK_COMMENT && state != ST_BLOCK_STAR) {
                    while (i < end) dc_push_code(out, &boundary, in[i++]);
                }
                i = end - 1;
                continue;
            }
        }

        switch (state) {

        case ST_CODE:
            if ((flags & CLIKE_GO_DIRECTIVES) && c == 'i' && go_imports_c(in, len, i)) {
                out->len = output_start;
                return dc_buf_append(out, in, len);
            }
            if (flags & CLIKE_REGEX_JS) {
                if (c == '(') {
                    if (paren_depth == sizeof(control_parens)) {
                        out->len = output_start;
                        return dc_buf_append(out, in, len);
                    }
                    control_parens[paren_depth++] = (unsigned char)regex_keyword(out->data, skip_space(out->data, out->len), 1);
                } else if (c == ')') {
                    control_paren = paren_depth > 0 ? control_parens[--paren_depth] : 0;
                }
            }
            if (c == '/' && (flags & (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT))) {
                size_t next = i + 1;
                if (flags & CLIKE_LINE_SPLICE) {
                    size_t end;
                    while ((end = splice_end(in, len, next)) != next) next = end;
                }
                if (next < len && in[next] == '/' && (flags & CLIKE_LINE_COMMENT)) {
                    if ((flags & CLIKE_GO_DIRECTIVES) && go_directive(in + next + 1, len - next - 1)) {
                        while (i < len && in[i] != '\n') dc_push_code(out, &boundary, in[i++]);
                        if (i < len) dc_push_code(out, &boundary, in[i]);
                    } else {
                        dc_boundary_start_comment(out, &boundary);
                        state = ST_LINE_COMMENT;
                        i = next;
                    }
                } else if (next < len && in[next] == '*' && (flags & CLIKE_BLOCK_COMMENT)) {
                    if ((flags & CLIKE_GO_DIRECTIVES) && len - next - 1 >= 5 &&
                        memcmp(in + next + 1, "line ", 5) == 0) {
                        size_t end = next + 1;
                        while (end + 1 < len && !(in[end] == '*' && in[end + 1] == '/')) end++;
                        end = end + 1 < len ? end + 2 : len;
                        while (i < end) dc_push_code(out, &boundary, in[i++]);
                        i--;
                    } else {
                        dc_boundary_start_comment(out, &boundary);
                        state = ST_BLOCK_COMMENT;
                        nest_depth = 1;
                        block_newlines = 0;
                        i = next;
                    }
                } else {
                    if ((flags & CLIKE_REGEX_JS) && can_start_regex(out, control_paren)) state = ST_REGEX;
                    dc_push_code(out, &boundary, c);
                }
            } else if (c == '#' && (flags & CLIKE_HASH_COMMENT)) {
                dc_boundary_start_comment(out, &boundary);
                state = ST_HASH_COMMENT;
            } else if (c == '"') {
                if ((flags & CLIKE_RAW_STRING_CPP) && out->len > 0 && out->data[out->len - 1] == 'R') {
                     
                    raw_cpp_delim_len = 0;
                    state = ST_RAW_CPP_PREFIX;
                    dc_push_code(out, &boundary, c);
                } else {
                    state = ST_STRING_DQ;
                    dc_push_code(out, &boundary, c);
                }
            } else if (c == 'r' && (flags & CLIKE_RAW_STRING_RUST) && i + 1 < len) {
                if (in[i + 1] == '"') {
                    rust_hash_count = 0;
                    dc_push_code(out, &boundary, 'r');
                    dc_push_code(out, &boundary, '"');
                    i++;
                    state = ST_RAW_RUST;
                } else if (in[i + 1] == '#') {
                    size_t j = i + 1;
                    int hc = 0;
                    while (j < len && in[j] == '#') { hc++; j++; }
                    if (j < len && in[j] == '"') {
                        rust_hash_count = hc;
                        dc_push_code(out, &boundary, 'r');
                        for (int h = 0; h < hc; h++) dc_push_code(out, &boundary, '#');
                        dc_push_code(out, &boundary, '"');
                        i = j;
                        state = ST_RAW_RUST;
                    } else {
                        dc_push_code(out, &boundary, c);
                    }
                } else {
                    dc_push_code(out, &boundary, c);
                }
            } else if (c == '\'' && (flags & (CLIKE_CHAR_LITERAL | CLIKE_SINGLE_STRING))) {
                state = ST_STRING_SQ;
                dc_push_code(out, &boundary, c);
            } else if (c == '`' && (flags & CLIKE_RAW_STRING_GO)) {
                state = ST_RAW_GO;
                dc_push_code(out, &boundary, c);
            } else if (c == '`' && (flags & CLIKE_TEMPLATE_LIT)) {
                state = ST_TEMPLATE_LIT;
                dc_push_code(out, &boundary, c);
            } else if (c == '{' && brace_top >= 0) {
                brace_depth[brace_top]++;
                dc_push_code(out, &boundary, c);
            } else if (c == '}' && brace_top >= 0 && brace_depth[brace_top] <= 0) {
                brace_top--;
                state = ST_TEMPLATE_LIT;
                dc_push_code(out, &boundary, c);
            } else if (c == '}' && brace_top >= 0) {
                brace_depth[brace_top]--;
                dc_push_code(out, &boundary, c);
            } else {
                dc_push_code(out, &boundary, c);
            }
            break;

        case ST_LINE_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
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
                            dc_push_comment_newline(out, &boundary);
                    } else {
                        dc_push_comment_space(out, &boundary);
                    }
                    if (boundary.suppress_comment_replacement && i + 1 < len && in[i + 1] == '\n')
                        i++;
                    dc_boundary_skip_leading_ws(&boundary);
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
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = ST_STRING_DQ_ESC;
            else if (c == '"') state = ST_CODE;
            else if (c == '\n' && (flags & CLIKE_CHAR_LITERAL)) {
                

                state = ST_CODE;
            }
            break;

        case ST_STRING_DQ_ESC:
            dc_push_code(out, &boundary, c);
            state = ST_STRING_DQ;
            break;

        case ST_STRING_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = ST_STRING_SQ_ESC;
            else if (c == '\'') state = ST_CODE;
            else if (c == '\n' && (flags & CLIKE_CHAR_LITERAL)) {
                state = ST_CODE;
            }
            break;

        case ST_STRING_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = ST_STRING_SQ;
            break;

        case ST_RAW_CPP_PREFIX:
            dc_push_code(out, &boundary, c);
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
            dc_push_code(out, &boundary, c);
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
            dc_push_code(out, &boundary, c);
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
            dc_push_code(out, &boundary, c);
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
            dc_push_code(out, &boundary, c);
            if (c == '`') state = ST_CODE;
            break;

        case ST_TEMPLATE_LIT:
            dc_push_code(out, &boundary, c);
            if (c == '\\') {
                state = ST_TEMPLATE_ESC;
            } else if (c == '`') {
                state = ST_CODE;
            } else if (c == '$' && i + 1 < len && in[i + 1] == '{') {
                dc_push_code(out, &boundary, '{');
                i++;
                if (brace_top < 62) {
                    brace_top++;
                    brace_depth[brace_top] = 0;
                }
                state = ST_CODE;
            }
            break;

        case ST_TEMPLATE_ESC:
            dc_push_code(out, &boundary, c);
            state = ST_TEMPLATE_LIT;
            break;

        case ST_REGEX:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = ST_REGEX_ESC;
            else if (c == '[') state = ST_REGEX_CLASS;
            else if (c == '/') {
                while (i + 1 < len && ((in[i + 1] >= 'a' && in[i + 1] <= 'z') ||
                                        (in[i + 1] >= 'A' && in[i + 1] <= 'Z'))) {
                    i++;
                    dc_push_code(out, &boundary, in[i]);
                }
                state = ST_CODE;
            } else if (c == '\n') {
                state = ST_CODE;
            }
            break;

        case ST_REGEX_ESC:
            dc_push_code(out, &boundary, c);
            state = ST_REGEX;
            break;

        case ST_REGEX_CLASS:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = ST_REGEX_CLASS_ESC;
            else if (c == ']') state = ST_REGEX;
            break;

        case ST_REGEX_CLASS_ESC:
            dc_push_code(out, &boundary, c);
            state = ST_REGEX_CLASS;
            break;

        case ST_HASH_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
                state = ST_CODE;
            }
            break;
        }
    }

     
    if (state == ST_BLOCK_COMMENT || state == ST_BLOCK_STAR) {
        for (size_t n = 0; n < block_newlines; n++)
            dc_push_comment_newline(out, &boundary);
    }

    dc_boundary_finish(out, &boundary);
    return DC_OK;
}
