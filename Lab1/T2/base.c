#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char my_get(char c) {
    return fgetc(stdin);
}

char cprt(char c) {
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c\n", c);
    } else {
        printf(".\n");
    }
    return c;
}

char encrypt(char c) {
    if (c >= 0x1F && c <= 0x7E) {
        return c + 1;
    }
    return c;
}

char decrypt(char c) {
    if (c >= 0x21 && c <= 0x7F) {
        return c - 1;
    }
    return c;
}

char xprt(char c) {
    printf("%02X\n", (unsigned char)c);
    return c;
}

char dprt(char c) {
    printf("%d\n", c);
    return c;
}

char* map(char *array, int array_length, char (*f) (char)){
    char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
    if (mapped_array == NULL) {
        return NULL;  // Handle allocation failure
    }
    
    for (int i = 0; i < array_length; i++) {
        mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}
 
int main(int argc, char **argv){
    // Test case 1: Testing xprt
    printf("Test case 1 - xprt function:\n");
    char arr1[] = {'H','E','Y','!'};
    char* arr2 = map(arr1, 4, xprt);
    free(arr2);
    
    // Test case 2: Testing multiple functions
    printf("\nTest case 2 - multiple functions:\n");
    int base_len = 4;
    char arr3[base_len];
    printf("Please enter %d characters: ", base_len);
    
    char* arr4 = map(arr3, base_len, my_get);
    char* arr5 = map(arr4, base_len, dprt);
    char* arr6 = map(arr5, base_len, xprt);
    char* arr7 = map(arr6, base_len, encrypt);
    
    // Clean up
    free(arr4);
    free(arr5);
    free(arr6);
    free(arr7);
    
    return 0;
}
