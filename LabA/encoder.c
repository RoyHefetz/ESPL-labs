#include <stdio.h>
#include <stdlib.h> 

FILE *infile;
FILE *outfile;
int debug_mode = 1;        // Debug mode on by default
const char *encoding_key = "0";  // Default key
int is_addition = 1;       // Default is addition
int key_index = 0;         // Current position in encoding key
int key_length = 1;        // Initialized for default input '0'

void process_file(const char* arg) {
    if (arg[0] == '-' && arg[1] == 'i') {  // Input file
        if ((infile = fopen(arg + 2, "r")) == NULL) {
            fprintf(stderr, "Error opening input file %s\n", arg + 2);
            if (outfile != stdout) {
                fclose(outfile);
            }
            exit(1);   // Terminate with error
        }
    }
    else if (arg[0] == '-' && arg[1] == 'o') {  // Output file
        if ((outfile = fopen(arg + 2, "w")) == NULL) {
            fprintf(stderr, "Error opening output file %s\n", arg + 2);
            if (infile != stdin) {
                fclose(infile);
            }
            exit(1);   // Terminate with error
        }
    }
}

void process_debug(const char* arg) {
    if (arg[0] == '+' && arg[1] == 'D' && arg[2] == '\0') {
        debug_mode = 1;
    } 
    else if (arg[0] == '-' && arg[1] == 'D' && arg[2] == '\0') {
        debug_mode = 0;
    }
}

void process_encoding(const char* arg) {
    if (arg[0] == '+' && arg[1] == 'E') {
        is_addition = 1;
        encoding_key = arg + 2;  // Skip "+E"
        key_length = 0;
        while(encoding_key[key_length] != '\0') {
            key_length++;
        }
    } 
    else if (arg[0] == '-' && arg[1] == 'E') {
        is_addition = 0;
        encoding_key = arg + 2;  // Skip "-E"
        key_length = 0;
        while(encoding_key[key_length] != '\0') {
            key_length++;
        }
    }
}

int is_letter(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int is_digit(char c) {
    return (c >= '0' && c <= '9');
}

char encode(char c) {
    // Only encode letter or digit characters
    if (is_letter(c) || is_digit(c)) {
        int key_digit = encoding_key[key_index] - '0';  // Convert char to int
        
        if (is_letter(c)) {
            char base = (c >= 'a' && c <= 'z') ? 'a' : 'A';
            if (is_addition) {
                return ((c - base + key_digit) % 26) + base;
            } 
            else {
                return ((c - base - key_digit + 26) % 26) + base;
            }
        } 
        else {  // is_digit(c) is true
            if (is_addition) {
                return ((c - '0' + key_digit) % 10) + '0';
            } 
            else {
                return ((c - '0' - key_digit + 10) % 10) + '0';
            }
        }
    }
    
    // For non-encodable characters
    if (encoding_key[key_index + 1] != '\0') {
        key_index++;
    } else {
        key_index = 0;
    }
    return c;
}

void process_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (debug_mode) {
            fprintf(stderr, "%d: %s\n", i, argv[i]);
        }
        if ((argv[i][0] == '+' || argv[i][0] == '-') && argv[i][1] == 'E') {
            process_encoding(argv[i]);
        } 
        else if (argv[i][0] == '-' && (argv[i][1] == 'i' || argv[i][1] == 'o')) {
            process_file(argv[i]);
        }
        else {
            process_debug(argv[i]);
        }
    }
}

void process_input() {
    int c;
    while ((c = fgetc(infile)) != EOF) {
        c = encode(c);
        fputc(c, outfile);
    }
}

int main(int argc, char *argv[]) {
    // Initialize default streams
    infile = stdin;
    outfile = stdout;
    
    process_args(argc, argv);
    process_input();
    
    // Close files if they were opened
    if (infile != stdin) {
        fclose(infile);
    }
    if (outfile != stdout) {
        fclose(outfile);
    }
    return 0;
}
