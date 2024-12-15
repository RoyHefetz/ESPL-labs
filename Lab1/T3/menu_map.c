#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct fun_desc { //declration
    char *name;
    char (*fun)(char);
};

char my_get(char c) {
    return fgetc(stdin);
}

char cprt(char c) { //print input as Characters 
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c\n", c);
    } else {
        printf(".\n");
    }
    return c;
}

char encrypt(char c) {
    if (c >= 0x20 && c <= 0x7E) {
        return c + 1;
    }
    return c;
}

char decrypt(char c) {
    if (c >= 0x20 && c <= 0x7E) {
        return c - 1;
    }
    return c;
}

char xprt(char c) { //print input in Hex
    if (c >= 0x20 && c <= 0x7E) {
        printf("%02X\n", (unsigned char)c);
    } else {
        printf(".\n");
    }
    return c;
}

char dprt(char c) { //print input in Decimal
    if (c >= 0x20 && c <= 0x7E) {
        printf("%d\n", c);
    } else {
        printf(".\n");
    }
    return c;
}

char* map(char *array, int array_length, char (*f)(char)) {
    char* mapped_array = (char*)(malloc(array_length * sizeof(char)));
    if (mapped_array == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < array_length; i++) {
        mapped_array[i] = f(array[i]);
    }
    
    return mapped_array;
}

struct fun_desc menu[] = { //new "class"
    {"Get string", my_get},
    {"Print Decimal", dprt},
    {"Print Hex", xprt},
    {"Print Character", cprt},
    {"Encrypt", encrypt},
    {"Decrypt", decrypt},
    {NULL, NULL}
};

int main(int argc, char **argv) {
    char *carray = (char*)malloc(5 * sizeof(char));
    memset(carray, '\0', 5);
    
    while(1) {
        printf("\nSelect operation from the following menu (ctrl^D for exit):\n");
        
        int bound = 0;
        for(int i = 0; menu[i].name != NULL; i++) {
            printf("%d)  %s\n", i, menu[i].name);
            bound++;
        }
        
        printf("Option : ");
        char input[256];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            free(carray);
            exit(0);
        }

        // Skip empty lines
        if (input[0] == '\n') {
            continue;
        }
        
        int choice = atoi(input); //reading the input out of the user keyboard
        
        if (choice >= 0 && choice < bound) {
            printf("\nWithin bounds\n");
            
            if (choice == 0) {  // Get string
                char temp[5] = {0};
                fgets(temp, 5, stdin);  // Read exactly 4 chars + null
                
                // Clear input buffer
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                
                // Copy to carray
                strncpy(carray, temp, 4);
                carray[4] = '\n';
            } 
            else {
                char* temp = map(carray, 5, menu[choice].fun);
                if (temp != NULL) {
                    free(carray);
                    carray = temp;
                    carray[4] = '\n';  // Ensure last char is newline
                }
            }
            
            printf("DONE.\n");
        } 
        else {
            printf("Not within bounds\n");
            free(carray);
            exit(0);
        }
    }
    
    return 0;
}