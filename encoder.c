#include <stdio.h>
#include <string.h>

// Global variables for file streams
FILE *infile = NULL;
FILE *outfile = NULL;
int debug_mode = 1;  // Debug mode on by default

char encode(char c) {
    return c;  // For now, just return the character unchanged
}

void process_debug_flag(const char* arg) {
    if (strcmp(arg, "+D") == 0) {
        debug_mode = 1;
    } else if (strcmp(arg, "-D") == 0) {
        debug_mode = 0;
    }
}

void process_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (debug_mode) {
            fprintf(stderr, "%d: %s\n", i, argv[i]);
        }
        process_debug_flag(argv[i]);
    }
}

void process_input() {
    int c;
    infile = stdin;   // Initialize to standard input
    outfile = stdout; // Initialize to standard output

    while ((c = fgetc(infile)) != EOF) {
        c = encode(c);
        fputc(c, outfile);
    }
}

int main(int argc, char *argv[]) {
    process_args(argc, argv);
    process_input();
    return 0;
}