#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 16

void PrintHex(unsigned char *buffer, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X", buffer[i]);
        if (i < length - 1) printf(" ");
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        PrintHex(buffer, bytesRead);
        if (bytesRead == BUFFER_SIZE) printf(" ");
    }
    printf("\n");

    fclose(file);
    return 0;
}