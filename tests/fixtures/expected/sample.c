#include <stdio.h>

 


int main(void) {
    
    int x = 42;  
    char *msg = "Hello, // not a comment";
    char *msg2 = "Hello, /* also not */ a comment";
    char c = '\''; 
    char d = '/';  

    


    printf("%s\n", msg);

    return 0; 
}
