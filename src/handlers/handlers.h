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

#endif  
