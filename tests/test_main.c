#include "test_framework.h"
#include "buf.h"
#include "decomment.h"

#include "handlers/handlers.h"

TEST(buf_init_free) {
    dc_buf_t b;
    dc_buf_init(&b);
    ASSERT(b.data == NULL);
    ASSERT_EQ(b.len, 0);
    dc_buf_free(&b);
}

TEST(buf_push) {
    dc_buf_t b;
    dc_buf_init(&b);
    dc_buf_push(&b, 'a');
    dc_buf_push(&b, 'b');
    dc_buf_push(&b, 'c');
    ASSERT_EQ(b.len, 3);
    ASSERT(b.data[0] == 'a');
    ASSERT(b.data[1] == 'b');
    ASSERT(b.data[2] == 'c');
    dc_buf_free(&b);
}

TEST(buf_append) {
    dc_buf_t b;
    dc_buf_init(&b);
    dc_buf_append(&b, "hello", 5);
    dc_buf_append_str(&b, " world");
    ASSERT_EQ(b.len, 11);
    ASSERT(memcmp(b.data, "hello world", 11) == 0);
    dc_buf_free(&b);
}

TEST(buf_detach) {
    dc_buf_t b;
    dc_buf_init(&b);
    dc_buf_append_str(&b, "test");
    size_t len;
    char *data = dc_buf_detach(&b, &len);
    ASSERT_EQ(len, 4);
    ASSERT(memcmp(data, "test", 4) == 0);
    ASSERT(b.data == NULL);
    free(data);
}

/* ====== C-like handler tests ====== */

/* Helper: strip C-like and check result */
static void check_clike(const char *input, const char *expected, uint32_t flags) {
    dc_buf_t out;
    dc_buf_init(&out);
    dc_result_t r = dc_strip_clike(input, strlen(input), &out, flags);
    ASSERT_EQ(r, DC_OK);
    /* NULL-terminate for comparison */
    dc_buf_push(&out, '\0');
    if (strcmp(out.data, expected) != 0) {
        printf("FAIL\n    input:    \"%s\"\n    expected: \"%s\"\n    got:      \"%s\"\n",
               input, expected, out.data);
        dc_buf_free(&out);
        g_tests_failed++;
        g_tests_passed--;
        return;
    }
    dc_buf_free(&out);
}

TEST(clike_line_comment) {
    check_clike("int x; // comment\n", "int x; \n", CLIKE_C);
}

TEST(clike_block_comment) {
    check_clike("int/* comment */x;", "int x;", CLIKE_C);
}

TEST(clike_block_comment_multiline) {
    check_clike("/* line1\nline2\nline3 */\ncode;", "\n\n\ncode;", CLIKE_C);
}

TEST(clike_string_with_slash) {
    check_clike("char *s = \"hello // world\";\n", "char *s = \"hello // world\";\n", CLIKE_C);
}

TEST(clike_string_with_block) {
    check_clike("char *s = \"hello /* world */\";\n", "char *s = \"hello /* world */\";\n", CLIKE_C);
}

TEST(clike_char_literal) {
    check_clike("char c = '\\'';\n", "char c = '\\'';\n", CLIKE_C);
}

TEST(clike_escaped_quote_in_string) {
    check_clike("char *s = \"he said \\\"hello\\\"\";\n",
                "char *s = \"he said \\\"hello\\\"\";\n", CLIKE_C);
}

TEST(clike_no_comment) {
    check_clike("int x = 5;\n", "int x = 5;\n", CLIKE_C);
}

TEST(clike_empty_input) {
    check_clike("", "", CLIKE_C);
}

TEST(clike_only_comment) {
    check_clike("// just a comment\n", "\n", CLIKE_C);
}

TEST(clike_nested_block_rust) {
    check_clike("/* outer /* inner */ still comment */\ncode;",
                " \ncode;", CLIKE_RUST);
}

TEST(clike_go_raw_string) {
    check_clike("s := `hello // world /* comment */`\n",
                "s := `hello // world /* comment */`\n", CLIKE_GO);
}

TEST(clike_rust_raw_string) {
    check_clike("let s = r#\"hello // world\"#;\n",
                "let s = r#\"hello // world\"#;\n", CLIKE_RUST);
}

TEST(clike_js_template_literal) {
    check_clike("let s = `hello // ${x} world`;\n",
                "let s = `hello // ${x} world`;\n", CLIKE_JS);
}

TEST(clike_js_regex) {
    check_clike("let re = /hello/g;\n",
                "let re = /hello/g;\n", CLIKE_JS);
}

TEST(clike_js_regex_vs_division) {
    check_clike("let x = a / b; // comment\n",
                "let x = a / b; \n", CLIKE_JS);
}

TEST(clike_js_regex_after_equals) {
    check_clike("let re = /pattern/gi;\n",
                "let re = /pattern/gi;\n", CLIKE_JS);
}

TEST(clike_css_block_only) {
    check_clike("body { /* color */ color: red; }\n",
                "body {   color: red; }\n", CLIKE_CSS);
}

TEST(clike_css_string_preserved) {
    check_clike("a::before { content: '/* not a comment */'; }\n",
                "a::before { content: '/* not a comment */'; }\n", CLIKE_CSS);
}

TEST(clike_multiple_comments) {
    check_clike("a; // c1\nb; // c2\n", "a; \nb; \n", CLIKE_C);
}

TEST(clike_block_at_eof) {
    check_clike("x; /* unterminated", "x; ", CLIKE_C);
}

TEST(clike_slash_at_eof) {
    check_clike("x = a /", "x = a /", CLIKE_C);
}

TEST(clike_adjacent_comments) {
    check_clike("/* a *//* b */x;", "  x;", CLIKE_C);
}

TEST(clike_cpp_raw_string) {
    check_clike("auto s = R\"delim(// not a comment /* also not */)delim\";\n",
                "auto s = R\"delim(// not a comment /* also not */)delim\";\n", CLIKE_CPP);
}

static void check_python(const char *input, const char *expected) {
    dc_buf_t out;
    dc_buf_init(&out);
    dc_result_t r = dc_strip_python(input, strlen(input), &out, 0);
    ASSERT_EQ(r, DC_OK);
    dc_buf_push(&out, '\0');
    if (strcmp(out.data, expected) != 0) {
        printf("FAIL\n    input:    \"%s\"\n    expected: \"%s\"\n    got:      \"%s\"\n",
               input, expected, out.data);
        dc_buf_free(&out);
        g_tests_failed++;
        g_tests_passed--;
        return;
    }
    dc_buf_free(&out);
}

TEST(python_line_comment) {
    check_python("x = 5  # assign\n", "x = 5  \n");
}

TEST(python_hash_in_string) {
    check_python("s = \"hello # world\"\n", "s = \"hello # world\"\n");
}

TEST(python_hash_in_single_string) {
    check_python("s = 'hello # world'\n", "s = 'hello # world'\n");
}

TEST(python_triple_dq_string) {
    check_python("s = \"\"\"hello # world\"\"\"\n", "s = \"\"\"hello # world\"\"\"\n");
}

TEST(python_triple_sq_string) {
    check_python("s = '''hello # world'''\n", "s = '''hello # world'''\n");
}

TEST(python_shebang) {
    check_python("#!/usr/bin/env python3\n# comment\nx = 1\n",
                 "#!/usr/bin/env python3\n\nx = 1\n");
}

TEST(python_escaped_quote) {
    check_python("s = \"he said \\\"hi\\\"\" # comment\n",
                 "s = \"he said \\\"hi\\\"\" \n");
}

TEST(python_raw_string) {
    check_python("s = r\"hello # world\"\n", "s = r\"hello # world\"\n");
}

TEST(python_fstring) {
    check_python("s = f\"hello {x}\" # comment\n", "s = f\"hello {x}\" \n");
}

TEST(python_triple_with_quotes) {
    /* """he said "hi" """ is a triple-quoted string containing: he said "hi" */
    check_python("s = \"\"\"he said \\\"hi\\\"\"\"\" # comment\n",
                 "s = \"\"\"he said \\\"hi\\\"\"\"\" \n");
}

TEST(python_empty_input) {
    check_python("", "");
}

TEST(python_no_comments) {
    check_python("x = 1\ny = 2\n", "x = 1\ny = 2\n");
}

/* ====== Shell handler tests ====== */

static void check_shell(const char *input, const char *expected) {
    dc_buf_t out;
    dc_buf_init(&out);
    dc_result_t r = dc_strip_shell(input, strlen(input), &out, 0);
    ASSERT_EQ(r, DC_OK);
    dc_buf_push(&out, '\0');
    if (strcmp(out.data, expected) != 0) {
        printf("FAIL\n    input:    \"%s\"\n    expected: \"%s\"\n    got:      \"%s\"\n",
               input, expected, out.data);
        dc_buf_free(&out);
        g_tests_failed++;
        g_tests_passed--;
        return;
    }
    dc_buf_free(&out);
}

TEST(shell_line_comment) {
    check_shell("echo hello # comment\n", "echo hello \n");
}

TEST(shell_hash_in_dq_string) {
    check_shell("echo \"hello # world\"\n", "echo \"hello # world\"\n");
}

TEST(shell_hash_in_sq_string) {
    check_shell("echo 'hello # world'\n", "echo 'hello # world'\n");
}

TEST(shell_shebang) {
    check_shell("#!/bin/bash\n# comment\necho hi\n",
                "#!/bin/bash\n\necho hi\n");
}

TEST(shell_dollar_sq) {
    check_shell("echo $'hello # world'\n", "echo $'hello # world'\n");
}

TEST(shell_escaped_hash) {
    check_shell("echo \\# not a comment\n", "echo \\# not a comment\n");
}

TEST(shell_sq_no_escape) {
    /* Shell single quotes have NO escape mechanism.
       'hello\' ends the string at the second quote. The backslash is literal. */
    check_shell("echo 'hello\\' # comment\n", "echo 'hello\\' \n");
}

/* ====== Hash handler tests ====== */

static void check_hash(const char *input, const char *expected, uint32_t flags) {
    dc_buf_t out;
    dc_buf_init(&out);
    dc_result_t r = dc_strip_hash(input, strlen(input), &out, flags);
    ASSERT_EQ(r, DC_OK);
    dc_buf_push(&out, '\0');
    if (strcmp(out.data, expected) != 0) {
        printf("FAIL\n    input:    \"%s\"\n    expected: \"%s\"\n    got:      \"%s\"\n",
               input, expected, out.data);
        dc_buf_free(&out);
        g_tests_failed++;
        g_tests_passed--;
        return;
    }
    dc_buf_free(&out);
}

TEST(hash_basic_comment) {
    check_hash("key = value # comment\n", "key = value \n", 0);
}

TEST(hash_in_dq_string) {
    check_hash("key = \"value # not\"\n", "key = \"value # not\"\n", 0);
}

TEST(hash_in_sq_string) {
    check_hash("key = 'value # not'\n", "key = 'value # not'\n", 0);
}

TEST(hash_yaml_no_escape_sq) {
    /* YAML single-quoted strings don't use backslash escapes */
    check_hash("key: 'it''s a test' # comment\n",
               "key: 'it''s a test' \n", (1u << 16));
}

/* ====== Lua handler tests ====== */

static void check_lua(const char *input, const char *expected) {
    dc_buf_t out;
    dc_buf_init(&out);
    dc_result_t r = dc_strip_lua(input, strlen(input), &out, 0);
    ASSERT_EQ(r, DC_OK);
    dc_buf_push(&out, '\0');
    if (strcmp(out.data, expected) != 0) {
        printf("FAIL\n    input:    \"%s\"\n    expected: \"%s\"\n    got:      \"%s\"\n",
               input, expected, out.data);
        dc_buf_free(&out);
        g_tests_failed++;
        g_tests_passed--;
        return;
    }
    dc_buf_free(&out);
}

TEST(lua_line_comment) {
    check_lua("x = 5 -- comment\n", "x = 5 \n");
}

TEST(lua_long_comment) {
    check_lua("--[[ long comment ]]x = 5\n", " x = 5\n");
}

TEST(lua_long_comment_eq) {
    /* --[=[ ... ]=] closes at first matching ]=] */
    check_lua("--[=[ long comment ]=]x = 5\n", " x = 5\n");
}

TEST(lua_long_comment_multiline) {
    check_lua("--[[\nline1\nline2\n]]code\n", "\n\n\ncode\n");
}

TEST(lua_string_with_dashes) {
    check_lua("s = \"hello -- world\"\n", "s = \"hello -- world\"\n");
}

TEST(lua_long_string) {
    check_lua("s = [[ hello -- world ]]\n", "s = [[ hello -- world ]]\n");
}

TEST(lua_long_string_eq) {
    check_lua("s = [=[ hello -- world ]=]\n", "s = [=[ hello -- world ]=]\n");
}

TEST(lua_single_dash) {
    check_lua("x = a - b\n", "x = a - b\n");
}

TEST(lua_empty) {
    check_lua("", "");
}

/* ====== Main ====== */

int main(void) {
    printf("decomment test suite\n");
    printf("====================\n");

    TEST_SUITE("Buffer");
    RUN_TEST(buf_init_free);
    RUN_TEST(buf_push);
    RUN_TEST(buf_append);
    RUN_TEST(buf_detach);

    TEST_SUITE("C-like handler");
    RUN_TEST(clike_line_comment);
    RUN_TEST(clike_block_comment);
    RUN_TEST(clike_block_comment_multiline);
    RUN_TEST(clike_string_with_slash);
    RUN_TEST(clike_string_with_block);
    RUN_TEST(clike_char_literal);
    RUN_TEST(clike_escaped_quote_in_string);
    RUN_TEST(clike_no_comment);
    RUN_TEST(clike_empty_input);
    RUN_TEST(clike_only_comment);
    RUN_TEST(clike_nested_block_rust);
    RUN_TEST(clike_go_raw_string);
    RUN_TEST(clike_rust_raw_string);
    RUN_TEST(clike_js_template_literal);
    RUN_TEST(clike_js_regex);
    RUN_TEST(clike_js_regex_vs_division);
    RUN_TEST(clike_js_regex_after_equals);
    RUN_TEST(clike_css_block_only);
    RUN_TEST(clike_css_string_preserved);
    RUN_TEST(clike_multiple_comments);
    RUN_TEST(clike_block_at_eof);
    RUN_TEST(clike_slash_at_eof);
    RUN_TEST(clike_adjacent_comments);
    RUN_TEST(clike_cpp_raw_string);

    TEST_SUITE("Python handler");
    RUN_TEST(python_line_comment);
    RUN_TEST(python_hash_in_string);
    RUN_TEST(python_hash_in_single_string);
    RUN_TEST(python_triple_dq_string);
    RUN_TEST(python_triple_sq_string);
    RUN_TEST(python_shebang);
    RUN_TEST(python_escaped_quote);
    RUN_TEST(python_raw_string);
    RUN_TEST(python_fstring);
    RUN_TEST(python_triple_with_quotes);
    RUN_TEST(python_empty_input);
    RUN_TEST(python_no_comments);

    TEST_SUITE("Shell handler");
    RUN_TEST(shell_line_comment);
    RUN_TEST(shell_hash_in_dq_string);
    RUN_TEST(shell_hash_in_sq_string);
    RUN_TEST(shell_shebang);
    RUN_TEST(shell_dollar_sq);
    RUN_TEST(shell_escaped_hash);
    RUN_TEST(shell_sq_no_escape);

    TEST_SUITE("Hash handler");
    RUN_TEST(hash_basic_comment);
    RUN_TEST(hash_in_dq_string);
    RUN_TEST(hash_in_sq_string);
    RUN_TEST(hash_yaml_no_escape_sq);

    TEST_SUITE("Lua handler");
    RUN_TEST(lua_line_comment);
    RUN_TEST(lua_long_comment);
    RUN_TEST(lua_long_comment_eq);
    RUN_TEST(lua_long_comment_multiline);
    RUN_TEST(lua_string_with_dashes);
    RUN_TEST(lua_long_string);
    RUN_TEST(lua_long_string_eq);
    RUN_TEST(lua_single_dash);
    RUN_TEST(lua_empty);

    TEST_REPORT();
}
