/* Normal string with comment-like content */
char *a = "// not a comment";
char *b = "/* not a comment */";
char *c = "unterminated /* comment";

/* Escaped quotes */
char *d = "she said \"hello\"";
char *e = "back\\slash";

/* Character literals */
char f = '"';
char g = '\'';
char h = '\\';
char i = '/';

/* Adjacent strings */
char *j = "hello" /* comment */ "world";

/* Empty string before comment */
char *k = ""; // comment

/* String with newline escape */
char *l = "line1\nline2";
