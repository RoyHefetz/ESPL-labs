#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10000
#define FILENAME_SIZE 128

typedef struct {
    char debug_mode;
    char display_mode;  // 0 for decimal, 1 for hexadecimal
    char file_name[FILENAME_SIZE];
    int unit_size;
    unsigned char mem_buf[BUFFER_SIZE];
    size_t mem_count;
} state;

struct menu_item {
    char *name;
    void (*fun)(state*);
};

// Format strings for different unit sizes
static char* hex_formats[] = {"%02hhX\n", "%04hX\n", "No such unit", "%08X\n"};
static char* dec_formats[] = {"%hhd\n", "%hd\n", "No such unit", "%d\n"};

// Helper function to print a value according to unit size and display mode
void print_unit(state *s, unsigned int value) {
    if (s->display_mode) {
        printf(hex_formats[s->unit_size-1], value);
    } else {
        printf(dec_formats[s->unit_size-1], value);
    }
}

// Helper function to read a unit of the specified size
unsigned int read_unit(unsigned char *buffer, int unit_size) {
    unsigned int value = 0;
    for(int i = 0; i < unit_size; i++) {
        value |= (buffer[i] << (8 * i));  // Little-endian read
    }
    return value;
}

//Task 0b
void toggle_debug_mode(state *s) {
    s->debug_mode = !s->debug_mode;
    printf("Debug flag now %s\n", s->debug_mode ? "on" : "off");
}

//Task 0b
void set_file_name(state *s) {
    printf("Enter file name: ");
    char temp_name[FILENAME_SIZE];
    if (fgets(temp_name, sizeof(temp_name), stdin) != NULL) {
        // Remove newline if present
        size_t len = strlen(temp_name);
        if (len > 0 && temp_name[len-1] == '\n') {
            temp_name[len-1] = '\0';
        }
        strncpy(s->file_name, temp_name, FILENAME_SIZE - 1);
        s->file_name[FILENAME_SIZE - 1] = '\0';  // Ensure null termination
        
        if (s->debug_mode) {
            fprintf(stderr, "Debug: file name set to '%s'\n", s->file_name);
        }
    }
}

//Task 0b
void set_unit_size(state *s) {
    printf("Enter unit size (1, 2, or 4): ");
    char input[256];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        int size = atoi(input);
        if (size == 1 || size == 2 || size == 4) {
            s->unit_size = size;
            if (s->debug_mode) {
                fprintf(stderr, "Debug: set size to %d\n", size);
            }
        } else {
            printf("Error: Invalid unit size. Unit size remains %d\n", s->unit_size);
        }
    }
}

//Task 1a
void load_into_memory(state *s) {
    if (strlen(s->file_name) == 0) {
        printf("Error: file name is empty\n");
        return;
    }

    FILE *file = fopen(s->file_name, "rb");
    if (file == NULL) {
        printf("Error: failed to open file %s\n", s->file_name);
        return;
    }

    printf("Please enter <location> <length>\n");
    char input[256];
    unsigned int location;
    int length;

    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%x %d", &location, &length) != 2) {
            printf("Error: invalid input format\n");
            fclose(file);
            return;
        }

        if (s->debug_mode) {
            fprintf(stderr, "Debug info:\n");
            fprintf(stderr, "file name: %s\n", s->file_name);
            fprintf(stderr, "location: 0x%X\n", location);
            fprintf(stderr, "length: %d\n", length);
        }

        // Calculate total bytes to read based on unit size
        size_t bytes_to_read = length * s->unit_size;
        if (bytes_to_read > BUFFER_SIZE) {
            printf("Error: request exceeds buffer size\n");
            fclose(file);
            return;
        }

        // Seek to the specified location
        if (fseek(file, location, SEEK_SET) != 0) {
            printf("Error: failed to seek to location 0x%X\n", location);
            fclose(file);
            return;
        }

        // Read the data into memory buffer
        size_t bytes_read = fread(s->mem_buf, 1, bytes_to_read, file);
        if (bytes_read != bytes_to_read) {
            printf("Error: failed to read requested number of bytes\n");
            fclose(file);
            return;
        }

        s->mem_count = bytes_read;
        printf("Loaded %d units into memory\n", length);
    }

    fclose(file);
}

//Task 1b
void toggle_display_mode(state *s) {
    s->display_mode = !s->display_mode;
    printf("Display flag now %s, %s representation\n", 
           s->display_mode ? "on" : "off",
           s->display_mode ? "hexadecimal" : "decimal");
}

//Task 1c
void file_display(state *s) {
    if (strlen(s->file_name) == 0) {
        printf("Error: file name is empty\n");
        return;
    }

    FILE *file = fopen(s->file_name, "rb");
    if (file == NULL) {
        printf("Error: failed to open file %s\n", s->file_name);
        return;
    }

    printf("Enter file offset and length\n");
    char input[256];
    unsigned int offset;
    int length;

    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%x %d", &offset, &length) != 2) {
            printf("Error: invalid input format\n");
            fclose(file);
            return;
        }

        if (fseek(file, offset, SEEK_SET) != 0) {
            printf("Error: failed to seek to offset 0x%X\n", offset);
            fclose(file);
            return;
        }

        printf("%s\n", s->display_mode ? "Hexadecimal" : "Decimal");
        printf("%s\n", s->display_mode ? "===========" : "=======");

        unsigned char buffer[8];  // Big enough for largest unit size (4)
        for (int i = 0; i < length; i++) {
            memset(buffer, 0, sizeof(buffer));  // Clear buffer
            size_t bytes_read = fread(buffer, 1, s->unit_size, file);
            if (bytes_read != s->unit_size) {
                printf("Error: failed to read complete unit at offset 0x%X\n", offset + i * s->unit_size);
                break;
            }

            // Read value based on unit size
            unsigned int value = 0;
            switch(s->unit_size) {
                case 1:
                    value = buffer[0];
                    break;
                case 2:
                    value = buffer[0] | (buffer[1] << 8);
                    break;
                case 4:
                    value = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
                    break;
            }
            print_unit(s, value);
        }
    }

    fclose(file);
}

//Task 1c
void memory_display(state *s) {
    printf("Enter address and length\n");
    char input[256];
    unsigned int addr;
    int length;

    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%x %d", &addr, &length) != 2) {
            printf("Error: invalid input format\n");
            return;
        }

        // Check if the requested memory range is within bounds
        if (addr + (length * s->unit_size) > BUFFER_SIZE) {
            printf("Error: Memory access out of bounds\n");
            return;
        }

        // Check if we're trying to read beyond loaded memory
        if (addr + (length * s->unit_size) > s->mem_count) {
            printf("Error: Trying to read beyond loaded memory\n");
            return;
        }

        printf("%s\n", s->display_mode ? "Hexadecimal" : "Decimal");
        printf("%s\n", s->display_mode ? "===========" : "=======");

        for (int i = 0; i < length; i++) {
            unsigned int value = read_unit(&s->mem_buf[addr + (i * s->unit_size)], s->unit_size);
            print_unit(s, value);
        }
    }
}

//Task 1d
void save_into_file(state *s) {
    if (strlen(s->file_name) == 0) {
        printf("Error: file name is empty\n");
        return;
    }

    // Open file for reading and writing, without truncating
    FILE *file = fopen(s->file_name, "rb+");
    if (file == NULL) {
        printf("Error: failed to open file %s\n", s->file_name);
        return;
    }

    printf("Please enter <source-address> <target-location> <length>\n");
    char input[256];
    unsigned int source_addr, target_location;
    int length;

    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%x %x %d", &source_addr, &target_location, &length) != 3) {
            printf("Error: invalid input format\n");
            fclose(file);
            return;
        }

        if (s->debug_mode) {
            fprintf(stderr, "Debug info:\n");
            fprintf(stderr, "source_addr: 0x%X\n", source_addr);
            fprintf(stderr, "target_location: 0x%X\n", target_location);
            fprintf(stderr, "length: %d\n", length);
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        
        // Check if target location is beyond file size
        if (target_location >= file_size) {
            printf("Error: target location is beyond file size\n");
            fclose(file);
            return;
        }

        // Set up source pointer
        unsigned char *source = (source_addr == 0) ? s->mem_buf : (unsigned char *)source_addr;

        // Seek to target location
        if (fseek(file, target_location, SEEK_SET) != 0) {
            printf("Error: failed to seek to target location\n");
            fclose(file);
            return;
        }

        // Write the data
        size_t bytes_to_write = length * s->unit_size;
        size_t bytes_written = fwrite(source, 1, bytes_to_write, file);
        
        if (bytes_written != bytes_to_write) {
            printf("Error: failed to write all bytes\n");
        }

        fclose(file);
    }
}

//Task 1e
void memory_modify(state *s) {
    printf("Please enter <location> <val>\n");
    char input[256];
    unsigned int location, val;

    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%x %x", &location, &val) != 2) {
            printf("Error: invalid input format\n");
            return;
        }

        if (s->debug_mode) {
            fprintf(stderr, "Debug info:\n");
            fprintf(stderr, "location: 0x%X\n", location);
            fprintf(stderr, "val: 0x%X\n", val);
        }

        // Check if location is valid given unit size
        if (location + s->unit_size > BUFFER_SIZE) {
            printf("Error: location out of bounds\n");
            return;
        }

        // Write value to memory buffer according to unit size
        switch(s->unit_size) {
            case 1:
                s->mem_buf[location] = (unsigned char)(val & 0xFF);
                break;
            case 2:
                if (location + 1 >= BUFFER_SIZE) {
                    printf("Error: not enough space for unit size 2\n");
                    return;
                }
                s->mem_buf[location] = (unsigned char)(val & 0xFF);
                s->mem_buf[location + 1] = (unsigned char)((val >> 8) & 0xFF);
                break;
            case 4:
                if (location + 3 >= BUFFER_SIZE) {
                    printf("Error: not enough space for unit size 4\n");
                    return;
                }
                s->mem_buf[location] = (unsigned char)(val & 0xFF);
                s->mem_buf[location + 1] = (unsigned char)((val >> 8) & 0xFF);
                s->mem_buf[location + 2] = (unsigned char)((val >> 16) & 0xFF);
                s->mem_buf[location + 3] = (unsigned char)((val >> 24) & 0xFF);
                break;
        }
    }
}

//Task 0b
void quit(state *s) {
    if (s->debug_mode) {
        printf("quitting\n");
    }
    exit(0);
}

void print_debug_info(state *s) {
    if (s->debug_mode) {
        fprintf(stderr, "\nDebug Info:\n");
        fprintf(stderr, "Unit Size: %d\n", s->unit_size);
        fprintf(stderr, "File Name: %s\n", s->file_name);
        fprintf(stderr, "Memory Count: %zu\n", s->mem_count);
    }
}

// Menu items array
struct menu_item menu[] = {
    {"Toggle Debug Mode", toggle_debug_mode},
    {"Set File Name", set_file_name},
    {"Set Unit Size", set_unit_size},
    {"Load Into Memory", load_into_memory},
    {"Toggle Display Mode", toggle_display_mode},
    {"File Display", file_display},
    {"Memory Display", memory_display},
    {"Save Into File", save_into_file},
    {"Memory Modify", memory_modify},
    {"Quit", quit},
    {NULL, NULL}
};

int main(int argc, char **argv) {
    // Initialize state
    state s = {
        .debug_mode = 0,
        .display_mode = 0,  // Initial state: decimal representation
        .unit_size = 1,
        .file_name = "",
        .mem_count = 0
    };

    while(1) {
        print_debug_info(&s);
        
        printf("\nChoose action:\n");
        int bound = 0;
        for(int i = 0; menu[i].name != NULL; i++) {
            printf("%d-%s\n", i, menu[i].name);
            bound++;
        }

        // Read input using fgets
        char input[256];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            quit(&s);  // Handle EOF (Ctrl+D)
        }

        // Skip empty lines
        if (input[0] == '\n') {
            continue;
        }

        int choice = atoi(input);
        if (choice >= 0 && choice < bound) {
            menu[choice].fun(&s);
        } else {
            printf("Invalid option!\n");
        }
    }

    return 0;
}