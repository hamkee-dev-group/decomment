#ifndef DC_HANDLERS_H
#define DC_HANDLERS_H

#include "decomment.h"
#include "buf.h"
#include <stddef.h>
#include <stdint.h>

 
#define CLIKE_LINE_COMMENT     (1u << 0)    
#define CLIKE_BLOCK_COMMENT    (1u << 1)    
#define CLIKE_NESTED_BLOCK     (1u << 2)    
#define CLIKE_CHAR_LITERAL     (1u << 3)    
#define CLIKE_SINGLE_STRING    (1u << 4)    
#define CLIKE_RAW_STRING_CPP   (1u << 5)    
#define CLIKE_RAW_STRING_RUST  (1u << 6)    
#define CLIKE_RAW_STRING_GO    (1u << 7)    
#define CLIKE_TEMPLATE_LIT     (1u << 8)    
#define CLIKE_REGEX_JS         (1u << 9)    
#define CLIKE_HASH_COMMENT     (1u << 10)   

 
#define CLIKE_C        (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_CHAR_LITERAL)
#define CLIKE_CPP      (CLIKE_C | CLIKE_RAW_STRING_CPP)
#define CLIKE_JAVA     (CLIKE_C)
#define CLIKE_CSHARP   (CLIKE_C)
#define CLIKE_GO       (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_CHAR_LITERAL | CLIKE_RAW_STRING_GO)
#define CLIKE_RUST     (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_NESTED_BLOCK | CLIKE_CHAR_LITERAL | CLIKE_RAW_STRING_RUST)
#define CLIKE_JS       (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_SINGLE_STRING | CLIKE_TEMPLATE_LIT | CLIKE_REGEX_JS)
#define CLIKE_TS       CLIKE_JS
#define CLIKE_SWIFT    (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_NESTED_BLOCK)
#define CLIKE_KOTLIN   (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_NESTED_BLOCK | CLIKE_CHAR_LITERAL)
#define CLIKE_SCALA    (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_NESTED_BLOCK | CLIKE_CHAR_LITERAL)
#define CLIKE_DART     (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_SINGLE_STRING | CLIKE_CHAR_LITERAL)
#define CLIKE_CSS      (CLIKE_BLOCK_COMMENT | CLIKE_SINGLE_STRING)
#define CLIKE_PHP      (CLIKE_LINE_COMMENT | CLIKE_BLOCK_COMMENT | CLIKE_SINGLE_STRING | CLIKE_HASH_COMMENT)

 
dc_result_t dc_strip_clike(const char *in, size_t len, dc_buf_t *out, uint32_t flags);
dc_result_t dc_strip_python(const char *in, size_t len, dc_buf_t *out, uint32_t flags);
dc_result_t dc_strip_shell(const char *in, size_t len, dc_buf_t *out, uint32_t flags);
dc_result_t dc_strip_hash(const char *in, size_t len, dc_buf_t *out, uint32_t flags);
dc_result_t dc_strip_lua(const char *in, size_t len, dc_buf_t *out, uint32_t flags);

typedef struct {
    size_t line_start;
    size_t trailing_start;
    int line_has_code;
    int saw_code;
    int trailing_active;
    int suppress_comment_replacement;
    int skip_leading_ws;
} dc_boundary_t;

static inline void dc_boundary_init(dc_boundary_t *b)
{
    b->line_start = 0;
    b->trailing_start = 0;
    b->line_has_code = 0;
    b->saw_code = 0;
    b->trailing_active = 0;
    b->suppress_comment_replacement = 0;
    b->skip_leading_ws = 0;
}

static inline dc_result_t dc_push_code(dc_buf_t *out, dc_boundary_t *b, char c)
{
    dc_result_t r;

    if (b->skip_leading_ws && !b->saw_code) {
        if (c == ' ' || c == '\t' || c == '\r')
            return DC_OK;
        if (c == '\n') {
            b->line_start = out->len;
            b->line_has_code = 0;
            b->skip_leading_ws = 0;
            return DC_OK;
        }
        b->skip_leading_ws = 0;
    }

    r = dc_buf_push(out, c);
    if (r != DC_OK)
        return r;

    if (c == '\n') {
        b->line_start = out->len;
        b->line_has_code = 0;
    } else if (c != ' ' && c != '\t' && c != '\r') {
        b->line_has_code = 1;
        b->saw_code = 1;
        b->trailing_active = 0;
    }

    return DC_OK;
}

static inline void dc_boundary_start_comment(dc_buf_t *out, dc_boundary_t *b)
{
    b->suppress_comment_replacement = 0;

    if (b->line_has_code) {
        b->trailing_active = 0;
        return;
    }

    if (!b->saw_code) {
        out->len = b->line_start;
        b->suppress_comment_replacement = 1;
    } else if (!b->trailing_active) {
        b->trailing_active = 1;
        b->trailing_start = b->line_start;
    }
}

static inline dc_result_t dc_push_comment_newline(dc_buf_t *out, dc_boundary_t *b)
{
    dc_result_t r;

    if (b->suppress_comment_replacement)
        return DC_OK;

    r = dc_buf_push(out, '\n');
    if (r != DC_OK)
        return r;
    b->line_start = out->len;
    b->line_has_code = 0;
    return DC_OK;
}

static inline dc_result_t dc_push_comment_space(dc_buf_t *out, dc_boundary_t *b)
{
    if (b->suppress_comment_replacement)
        return DC_OK;
    return dc_buf_push(out, ' ');
}

static inline void dc_boundary_finish(dc_buf_t *out, dc_boundary_t *b)
{
    if (b->trailing_active)
        out->len = b->trailing_start;
}

static inline void dc_boundary_skip_leading_ws(dc_boundary_t *b)
{
    if (b->suppress_comment_replacement && !b->saw_code)
        b->skip_leading_ws = 1;
}

#endif  
