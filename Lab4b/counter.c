#include <stdio.h>

int count_digits(char* str) {
    int count = 0;
    int state = 1;  // This matches the original binary's behavior
    
    while(*str != '\0') {
        if(*str >= '0' && *str <= '9') { 
            count++;
        }
        str++;
    }
    return count;
}

int main(int argc, char** argv) {
    if(argc != 2) {
        return 1;
    }
    int result = count_digits(argv[1]);
    printf("String contains %d digits, right???\n", result);
    return 0;
}