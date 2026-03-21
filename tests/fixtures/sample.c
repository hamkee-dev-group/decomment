#include <stdio.h>

/* This is a block comment */
// This is a line comment

int main(void) {
    // Initialize variables
    int x = 42; /* inline block comment */
    char *msg = "Hello, // not a comment";
    char *msg2 = "Hello, /* also not */ a comment";
    char c = '\''; // char literal with escaped quote
    char d = '/';  // slash in char literal

    /* Multi-line
       block
       comment */
    printf("%s\n", msg);

    return 0; // exit
}
