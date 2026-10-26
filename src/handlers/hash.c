#include "handlers.h"

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
    int yaml = (flags & HASH_YAML) != 0;
    int yaml_plain = 0;
    int yaml_quoted = 0;
    size_t yaml_flow_depth = 0;
    size_t yaml_block_indent = SIZE_MAX;
    dc_boundary_t boundary;

    r = dc_buf_ensure(out, len);
    if (r != DC_OK) return r;
    dc_boundary_init(&boundary);

    for (size_t i = 0; i < len; i++) {
        char c = in[i];

        if (yaml && state == HASH_CODE && yaml_block_indent != SIZE_MAX &&
            (i == 0 || in[i - 1] == '\n')) {
            size_t end = i;
            while (end < len && in[end] == ' ') end++;
            if (end == len || in[end] == '\n' || in[end] == '\r' || end - i > yaml_block_indent) {
                while (i < len && in[i] != '\n') dc_push_code(out, &boundary, in[i++]);
                if (i < len) dc_push_code(out, &boundary, in[i]);
                continue;
            }
            yaml_block_indent = SIZE_MAX;
        }

        switch (state) {

        case HASH_CODE:
            if (c == '#' && (!yaml || i == 0 || in[i - 1] == ' ' ||
                in[i - 1] == '\t' || in[i - 1] == '\r' || in[i - 1] == '\n' || yaml_quoted)) {
                 
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
            } else if (yaml && !yaml_plain && (c == '!' || c == '&')) {
                dc_push_code(out, &boundary, c);
                if (c == '!' && i + 1 < len && in[i + 1] == '<') {
                    do {
                        dc_push_code(out, &boundary, in[++i]);
                    } while (i + 1 < len && in[i] != '>');
                } else {
                    while (i + 1 < len && in[i + 1] != ' ' && in[i + 1] != '\t' &&
                           in[i + 1] != '\r' && in[i + 1] != '\n' && in[i + 1] != ',' &&
                           in[i + 1] != '[' && in[i + 1] != ']' && in[i + 1] != '{' && in[i + 1] != '}')
                        dc_push_code(out, &boundary, in[++i]);
                }
                yaml_quoted = 0;
            } else if (c == '\'' && (!yaml || !yaml_plain)) {
                state = HASH_STRING_SQ;
                yaml_quoted = 0;
                dc_push_code(out, &boundary, c);
            } else if (c == '"' && (!yaml || !yaml_plain)) {
                state = HASH_STRING_DQ;
                yaml_quoted = 0;
                dc_push_code(out, &boundary, c);
            } else {
                dc_push_code(out, &boundary, c);
                if (c == '\n') is_first_line = 0;
                if (yaml) {
                    int was_quoted = yaml_quoted;
                    yaml_quoted = 0;
                    if ((c == '|' || c == '>') && !yaml_plain && yaml_flow_depth == 0) {
                        size_t end = i + 1;
                        while (end < len && (in[end] == '+' || in[end] == '-' ||
                               (in[end] >= '1' && in[end] <= '9'))) end++;
                        while (end < len && (in[end] == ' ' || in[end] == '\t' || in[end] == '\r')) end++;
                        if (end == len || in[end] == '\n' || in[end] == '#') {
                            size_t start = i;
                            while (start > 0 && in[start - 1] != '\n') start--;
                            size_t indent = start;
                            while (indent < i && in[indent] == ' ') indent++;
                            yaml_block_indent = indent - start;
                        }
                    }
                    if (c == '\n' || c == '\r') {
                        yaml_plain = 0;
                    } else if ((c == '[' || c == '{') && !yaml_plain) {
                        yaml_flow_depth++;
                    } else if ((c == ']' || c == '}') && yaml_flow_depth > 0) {
                        yaml_flow_depth--;
                        yaml_plain = 0;
                        yaml_quoted = 1;
                    } else if ((c == ':' && ((yaml_flow_depth > 0 && was_quoted) ||
                                i + 1 == len || in[i + 1] == ' ' || in[i + 1] == '\t' ||
                                in[i + 1] == '\r' || in[i + 1] == '\n')) ||
                               (c == ',' && yaml_flow_depth > 0) ||
                               ((c == '-' || c == '?') && !yaml_plain && i + 1 < len && in[i + 1] == ' ')) {
                        yaml_plain = 0;
                    } else if (c != ' ' && c != '\t') {
                        yaml_plain = 1;
                    }
                }
            }
            break;

        case HASH_COMMENT:
            if (c == '\n') {
                dc_push_comment_newline(out, &boundary);
                state = HASH_CODE;
                is_first_line = 0;
                yaml_plain = 0;
                yaml_quoted = 0;
            }
            break;

        case HASH_STRING_SQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\' && !sq_no_escape) state = HASH_STRING_SQ_ESC;
            else if (c == '\'') {
                if (yaml && i + 1 < len && in[i + 1] == '\'') {
                    dc_push_code(out, &boundary, in[++i]);
                } else {
                    state = HASH_CODE;
                    yaml_quoted = 1;
                }
            }
            break;

        case HASH_STRING_SQ_ESC:
            dc_push_code(out, &boundary, c);
            state = HASH_STRING_SQ;
            break;

        case HASH_STRING_DQ:
            dc_push_code(out, &boundary, c);
            if (c == '\\') state = HASH_STRING_DQ_ESC;
            else if (c == '"') {
                state = HASH_CODE;
                yaml_quoted = 1;
            }
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
