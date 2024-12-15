#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10240

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

typedef struct link link;
struct link {
    link *nextVirus;
    virus *vir;
};

typedef struct virus_location {
    int offset;
    char name[16];
} virus_location;

int is_little_endian = 1;  // Default to little endian

// Function declarations
virus* readVirus(FILE* file);
void printVirus(virus* virus, FILE* output);
int checkMagicNumber(FILE* file);
void list_print(link *virus_list, FILE* output);
link* list_append(link* virus_list, virus* data);
void list_free(link *virus_list);
link* load_signatures(char* filename);
void print_menu();
void detect_virus(char *buffer, unsigned int size, link *virus_list, virus_location* locations, int* count);
void neutralize_virus(char *fileName, int signatureOffset);


unsigned short convert_endian(unsigned short num) {
    if (is_little_endian) {
        return num;
    }
    return (num >> 8) | (num << 8);  // Swap bytes for big endian
}

virus* readVirus(FILE* file) {
    virus* v = malloc(sizeof(virus));
    if (v == NULL) return NULL;
    
    unsigned short size;
    if (fread(&size, sizeof(unsigned short), 1, file) != 1) {
        free(v);
        return NULL;
    }
    v->SigSize = convert_endian(size);
    
    if (fread(v->virusName, sizeof(char), 16, file) != 16) {
        free(v);
        return NULL;
    }
    
    v->sig = malloc(v->SigSize);
    if (v->sig == NULL) {
        free(v);
        return NULL;
    }
    
    if (fread(v->sig, sizeof(unsigned char), v->SigSize, file) != v->SigSize) {
        free(v->sig);
        free(v);
        return NULL;
    }
    
    return v;
}

void printVirus(virus* virus, FILE* output) {
    if (!virus || !output) return;
    
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);
    fprintf(output, "signature: ");
    
    for (int i = 0; i < virus->SigSize; i++) {
        fprintf(output, "%02X ", virus->sig[i]);
    }
    fprintf(output, "\n\n");
}

int checkMagicNumber(FILE* file) {
    char magic[4];
    if (fread(magic, 1, 4, file) != 4) return 0;
    
    if (memcmp(magic, "VIRL", 4) == 0) {
        is_little_endian = 1;
        return 1;
    }
    if (memcmp(magic, "VIRB", 4) == 0) {
        is_little_endian = 0;
        return 1;
    }
    return 0;
}

// Function to detect viruses in a buffer by comparing with known signatures
void detect_virus(char *buffer, unsigned int size, link *virus_list, virus_location* locations, int* count) {
    link *current = virus_list;
    unsigned char* ubuffer = (unsigned char*)buffer;
    *count = 0;
    
    while (current != NULL) {
        virus *v = current->vir;
        // Compare buffer content with virus signature
        for (unsigned int i = 0; i <= size - v->SigSize; i++) {
            if (memcmp(ubuffer + i, v->sig, v->SigSize) == 0) {
                printf("Virus detected!\n");
                printf("Starting byte location: %d\n", i);
                printf("Virus name: %s\n", v->virusName);
                printf("Virus size: %d\n\n", v->SigSize);
                
                // Store virus location for later neutralization
                locations[*count].offset = i;
                strncpy(locations[*count].name, v->virusName, 16);
                (*count)++;
            }
        }
        current = current->nextVirus;
    }
}

// Function to neutralize a detected virus by replacing its first byte with RET instruction
void neutralize_virus(char *fileName, int signatureOffset) {
    FILE* file = fopen(fileName, "r+b");
    if(file == NULL) {
        printf("Failed to open suspected file\n");
        return;
    }

    if(fseek(file, signatureOffset, SEEK_SET) != 0) {
        printf("Error seeking to position %d\n", signatureOffset);
        fclose(file);
        return;
    }

    // Replace first byte with RET instruction (0xC3)
    unsigned char ret = 0xC3;
    if(fwrite(&ret, 1, 1, file) != 1) {
        printf("Error writing RET instruction\n");
    } else {
        printf("Virus neutralized at offset %d\n", signatureOffset);
    }
    
    fclose(file);
}


void list_print(link *virus_list, FILE* output) {
    link *current = virus_list;
    while (current != NULL) {
        printVirus(current->vir, output);
        current = current->nextVirus;
    }
}

link* list_append(link* virus_list, virus* data) {
    link* new_link = malloc(sizeof(link));
    new_link->vir = data;
    new_link->nextVirus = NULL;
    
    if (virus_list == NULL) {
        return new_link;
    }
    
    link* current = virus_list;
    while (current->nextVirus != NULL) {
        current = current->nextVirus;
    }
    current->nextVirus = new_link;
    return virus_list;
}

void list_free(link *virus_list) {
    link *current = virus_list;
    while (current != NULL) {
        link *next = current->nextVirus;
        free(current->vir->sig);
        free(current->vir);
        free(current);
        current = next;
    }
}

link* load_signatures(char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open signatures file\n");
        return NULL;
    }
    
    if (!checkMagicNumber(file)) {
        printf("Invalid magic number\n");
        fclose(file);
        return NULL;
    }
    
    link* virus_list = NULL;
    virus* v;
    while ((v = readVirus(file)) != NULL) {
        virus_list = list_append(virus_list, v);
    }
    
    fclose(file);
    return virus_list;
}

void print_menu() {
    printf("\nVirus Detector Menu:\n");
    printf("1) Load signatures\n");
    printf("2) Print signatures\n");
    printf("3) Detect viruses\n");
    printf("4) Fix file\n");
    printf("5) Quit\n");
    printf("Please choose an option: ");
}

int main() {
    link* virus_list = NULL;
    char buffer[BUFFER_SIZE];
    int option;
    
    while(1) {
        print_menu();
        fgets(buffer, BUFFER_SIZE, stdin);
        sscanf(buffer, "%d", &option);
        
        switch(option) {
            case 1: {
                printf("Enter signatures file name: ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                if (virus_list != NULL) {
                    list_free(virus_list);
                }
                virus_list = load_signatures(buffer);
                break;
            }
            case 2: {
                if (virus_list != NULL) {
                    printf("Printing signatures:\n");
                    list_print(virus_list, stdout);
                } else {
                    printf("No signatures loaded\n");
                }
                break;
            }
            case 3: {
                char filename[100];
                printf("Enter suspected file name: ");
                fgets(filename, 100, stdin);
                filename[strcspn(filename, "\n")] = 0;

                FILE *suspected = fopen(filename, "rb");
                if (suspected == NULL) {
                    printf("Failed to open suspected file\n");
                    break;
                }

                // Read file content into buffer
                char buffer[BUFFER_SIZE];
                size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, suspected);
                fclose(suspected);
                
                if (bytesRead > 0 && virus_list != NULL) {
                    virus_location locations[10];
                    int count = 0;
                    detect_virus(buffer, bytesRead, virus_list, locations, &count);
                }
                break;
            }


            case 4: {
                char filename[100];
                printf("Enter suspected file name: ");
                fgets(filename, 100, stdin);
                filename[strcspn(filename, "\n")] = 0;

                FILE *suspected = fopen(filename, "rb");
                if (suspected == NULL) {
                    printf("Failed to open suspected file\n");
                    break;
                }

                // Read and scan file for viruses
                char buffer[BUFFER_SIZE];
                size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, suspected);
                fclose(suspected);
                
                if (bytesRead > 0 && virus_list != NULL) {
                    virus_location locations[10];
                    int count = 0;
                    detect_virus(buffer, bytesRead, virus_list, locations, &count);
                    
                    // Neutralize all detected viruses
                    for (int i = 0; i < count; i++) {
                        printf("Neutralizing virus: %s at offset %d\n", 
                            locations[i].name, locations[i].offset);
                        neutralize_virus(filename, locations[i].offset);
                    }
                }
                break;
            }

            case 5:
                if (virus_list != NULL) {
                    list_free(virus_list);
                }
                exit(0);
            default:
                printf("Invalid option\n");
        }
    }
    
    return 0;
}
