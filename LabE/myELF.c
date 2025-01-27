#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

typedef struct {
    int dbg;
    int fd1;
    int fd2;
    void *map1;
    void *map2;
    size_t size1;
    size_t size2;
    char name1[256];
    char name2[256];
} ElfState;

ElfState state = {0, -1, -1, NULL, NULL, 0, 0};

struct MenuOption {
    char *name;
    void (*func)(ElfState*);
};

// Function declarations
void toggle_debug(ElfState* s);
void examine_elf(ElfState* s);
void print_sections(ElfState* s);
void print_symbols(ElfState* s);
void check_merge(ElfState* s);
void merge_files(ElfState* s);
void quit(ElfState* s);

const struct MenuOption menu[] = {
    {"Toggle Debug Mode", toggle_debug},
    {"Examine ELF File", examine_elf},
    {"Print Section Names", print_sections},
    {"Print Symbols", print_symbols},
    {"Check Files for Merge", check_merge},
    {"Merge ELF Files", merge_files},
    {"Quit", quit},
};

// Part 0 functions
void toggle_debug(ElfState* s) {
    s->dbg = !s->dbg;
    printf("Debug mode %s\n", s->dbg ? "on" : "off");
}

const char* get_section_type(uint32_t type) {
    static const struct {
        uint32_t type;
        const char* name;
    } types[] = {
        {SHT_NULL, "NULL"},
        {SHT_PROGBITS, "PROGBITS"},
        {SHT_SYMTAB, "SYMTAB"},
        {SHT_STRTAB, "STRTAB"},
        {SHT_RELA, "RELA"},
        {SHT_HASH, "HASH"},
        {SHT_DYNAMIC, "DYNAMIC"},
        {SHT_NOTE, "NOTE"},
        {SHT_NOBITS, "NOBITS"},
        {SHT_REL, "REL"},
        {SHT_SHLIB, "SHLIB"},
        {SHT_DYNSYM, "DYNSYM"},
        {SHT_INIT_ARRAY, "INIT_ARRAY"},
        {SHT_FINI_ARRAY, "FINI_ARRAY"},
        {SHT_PREINIT_ARRAY, "PREINIT_ARRAY"},
        {SHT_GROUP, "GROUP"},
        {SHT_SYMTAB_SHNDX, "SYMTAB_SHNDX"}
    };
    
    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        if (types[i].type == type) return types[i].name;
    }
    return "UNKNOWN";
}

void examine_elf(ElfState* s) {
    if (s->fd1 != -1 && s->fd2 != -1) {
        printf("Two ELF files are already open. Cannot open more files.\n");
        return;
    }

    printf("Enter ELF file name: ");
    char fname[256];
    fgets(fname, sizeof(fname), stdin);
    fname[strcspn(fname, "\n")] = '\0';

    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        perror("Error: Failed to open file");
        return;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        perror("Error: Failed to get file size");
        close(fd);
        return;
    }

    void *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("Error: Failed to map file");
        close(fd);
        return;
    }

    Elf32_Ehdr *hdr = (Elf32_Ehdr *)map;
    if (strncmp((char*)hdr->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Error: Not an ELF file\n");
        munmap(map, size);
        close(fd);
        return;
    }

    printf("\n");
    printf("Magic: %c%c%c\n", hdr->e_ident[1], hdr->e_ident[2], hdr->e_ident[3]);
    printf("Data:%s\n", hdr->e_ident[EI_DATA] == ELFDATA2LSB ? "2's complement, little endian" : "Unknown");
    printf("Entry point: 0x%x\n", hdr->e_entry);
    printf("Section header offset: %d\n", hdr->e_shoff);
    printf("Number of section headers: %d\n", hdr->e_shnum);
    printf("Size of section header: %d\n", hdr->e_shentsize);
    printf("Program header offset: %d\n", hdr->e_phoff);
    printf("Number of program headers: %d\n", hdr->e_phnum);
    printf("Size of program header: %d\n", hdr->e_phentsize);

    if (s->fd1 == -1) {
        s->fd1 = fd;
        s->map1 = map;
        s->size1 = size;
        strcpy(s->name1, fname);
    } else {
        s->fd2 = fd;
        s->map2 = map;
        s->size2 = size;
        strcpy(s->name2, fname);
    }
}

// Part 1 function
void print_sections(ElfState* s) {
    if (s->fd1 == -1 && s->fd2 == -1) {
        printf("Error: No ELF files opened. Use 'Examine ELF File' first.\n");
        return;
    }

    for (int i = 0; i < 2; i++) {
        int fd = (i == 0) ? s->fd1 : s->fd2;
        void *map = (i == 0) ? s->map1 : s->map2;
        if (fd == -1) continue;

        Elf32_Ehdr *hdr = (Elf32_Ehdr *)map;
        Elf32_Shdr *sections = (Elf32_Shdr *)(map + hdr->e_shoff);
        Elf32_Shdr *shstrtab = &sections[hdr->e_shstrndx];
        const char *strtab = (const char *)(map + shstrtab->sh_offset);

        printf("\nFile: %s\n", (i == 0) ? s->name1 : s->name2);

        if (s->dbg) {
            printf("Debug: ELF header details:\n");
            printf("  e_shoff: %x\n", hdr->e_shoff);
            printf("  e_shnum: %d\n", hdr->e_shnum);
            printf("  e_shstrndx: %d\n", hdr->e_shstrndx);
            printf("Debug: shstrtab details:\n");
            printf("  shstrtab_offset: %x\n", shstrtab->sh_offset);
            printf("  shstrtab_size: %x\n", shstrtab->sh_size);
        }

        printf("[index] section_name             section_address section_offset section_size section_type\n");

        for (int j = 0; j < hdr->e_shnum; j++) {
            printf("[%2d] %-24s 0x%08x      0x%06x         0x%06x       %s\n",
                j,
                &strtab[sections[j].sh_name],
                sections[j].sh_addr,
                sections[j].sh_offset,
                sections[j].sh_size,
                get_section_type(sections[j].sh_type));
        }
    }
}

// Helper function for Parts 2 and 3
const char* get_section_name(Elf32_Ehdr *hdr, int idx) {
    if (idx >= 0 && idx < hdr->e_shnum) {
        Elf32_Shdr *sections = (Elf32_Shdr *)((char *)hdr + hdr->e_shoff);
        Elf32_Shdr *shstrtab = &sections[hdr->e_shstrndx];
        char *strtab = (char *)hdr + shstrtab->sh_offset;
        return strtab + sections[idx].sh_name;
    }
    return "Unavailable";
}

// Part 2 function
void print_symbols(ElfState* s) {
    if (s->fd1 == -1 && s->fd2 == -1) {
        printf("No ELF files opened. Use 'Examine ELF File' first.\n");
        return;
    }

    for (int i = 0; i < 2; i++) {
        void *map = (i == 0) ? s->map1 : s->map2;
        int fd = (i == 0) ? s->fd1 : s->fd2;
        if (fd == -1) continue;

        Elf32_Ehdr *hdr = (Elf32_Ehdr *)map;
        Elf32_Shdr *sections = (Elf32_Shdr *)((char *)hdr + hdr->e_shoff);
        Elf32_Shdr *symtab = NULL;
        Elf32_Shdr *strtab = NULL;

        for (int j = 0; j < hdr->e_shnum; j++) {
            if (sections[j].sh_type == SHT_SYMTAB) {
                symtab = &sections[j];
                strtab = &sections[sections[j].sh_link];
                break;
            }
        }

        if (!symtab || !strtab) {
            printf("No symbol table found in ELF file.\n");
            continue;
        }

        int sym_count = symtab->sh_size / sizeof(Elf32_Sym);
        Elf32_Sym *syms = (Elf32_Sym *)((char *)hdr + symtab->sh_offset);
        const char *str_table = (const char *)((char *)hdr + strtab->sh_offset);

        if (s->dbg) {
            printf("Debug: Symbol table size: %d\n", symtab->sh_size);
            printf("Debug: Number of symbols: %d\n", sym_count);
        }

        printf("\nFile: %s\n", (i == 0) ? s->name1 : s->name2);
        printf("[index] value section_index section_name symbol_name\n");

        for (int j = 0; j < sym_count; j++) {
            printf("[%2d] 0x%08x %d %s %s\n",
                j,
                syms[j].st_value,
                syms[j].st_shndx,
                get_section_name(hdr, syms[j].st_shndx),
                str_table + syms[j].st_name);
        }
    }
}

// Part 3 functions
void check_merge(ElfState* s) {
    if (s->fd1 == -1 || s->fd2 == -1) {
        printf("Two ELF files must be open for merging.\n");
        return;
    }

    Elf32_Ehdr *hdr1 = (Elf32_Ehdr *)s->map1;
    Elf32_Ehdr *hdr2 = (Elf32_Ehdr *)s->map2;
    Elf32_Shdr *sections1 = (Elf32_Shdr *)((char *)hdr1 + hdr1->e_shoff);
    Elf32_Shdr *sections2 = (Elf32_Shdr *)((char *)hdr2 + hdr2->e_shoff);
    Elf32_Shdr *symtab1 = NULL, *symtab2 = NULL;

    // Find symbol tables
    for (int i = 0; i < hdr1->e_shnum; i++) {
        if (sections1[i].sh_type == SHT_SYMTAB) {
            if (symtab1) {
                printf("Multiple symbol tables found in first file.\n");
                return;
            }
            symtab1 = &sections1[i];
        }
    }

    for (int i = 0; i < hdr2->e_shnum; i++) {
        if (sections2[i].sh_type == SHT_SYMTAB) {
            if (symtab2) {
                printf("Multiple symbol tables found in second file.\n");
                return;
            }
            symtab2 = &sections2[i];
        }
    }

    if (!symtab1 || !symtab2) {
        printf("Symbol table missing in one or both files.\n");
        return;
    }

    Elf32_Sym *syms1 = (Elf32_Sym *)((char *)hdr1 + symtab1->sh_offset);
    Elf32_Sym *syms2 = (Elf32_Sym *)((char *)hdr2 + symtab2->sh_offset);
    const char *strtab1 = (const char *)((char *)hdr1 + sections1[symtab1->sh_link].sh_offset);
    const char *strtab2 = (const char *)((char *)hdr2 + sections2[symtab2->sh_link].sh_offset);
    int sym_count1 = symtab1->sh_size / sizeof(Elf32_Sym);
    int sym_count2 = symtab2->sh_size / sizeof(Elf32_Sym);

    for (int i = 1; i < sym_count1; i++) {
        const char *name1 = strtab1 + syms1[i].st_name;
        
        if (ELF32_ST_BIND(syms1[i].st_info) == STB_GLOBAL) {
            if (syms1[i].st_shndx == SHN_UNDEF) {
                // Check undefined symbols
                int defined = 0;
                for (int j = 1; j < sym_count2; j++) {
                    const char *name2 = strtab2 + syms2[j].st_name;
                    if (strcmp(name1, name2) == 0 && syms2[j].st_shndx != SHN_UNDEF) {
                        defined = 1;
                        break;
                    }
                }
                if (!defined) printf("Symbol %s undefined\n", name1);
            } else {
                // Check multiply defined symbols
                for (int j = 1; j < sym_count2; j++) {
                    const char *name2 = strtab2 + syms2[j].st_name;
                    if (strcmp(name1, name2) == 0 && syms2[j].st_shndx != SHN_UNDEF) {
                        printf("Symbol %s multiply defined\n", name1);
                        break;
                    }
                }
            }
        }
    }
}

void merge_files(ElfState* s) {
    if (s->fd1 == -1 || s->fd2 == -1) {
        printf("Two ELF files must be open for merging.\n");
        return;
    }

    int outfd = open("out.ro", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (outfd == -1) {
        perror("Failed to create output file");
        return;
    }

    Elf32_Ehdr hdr1 = *(Elf32_Ehdr*)s->map1;
    if (hdr1.e_shoff >= s->size1) {
        close(outfd);
        return;
    }

    Elf32_Shdr* sections1 = (Elf32_Shdr*)(s->map1 + hdr1.e_shoff);
    Elf32_Ehdr* hdr2 = (Elf32_Ehdr*)s->map2;
    if (hdr2->e_shoff >= s->size2) {
        close(outfd);
        return;
    }
    Elf32_Shdr* sections2 = (Elf32_Shdr*)(s->map2 + hdr2->e_shoff);

    // Write initial headers
    hdr1.e_shoff = 0x34;
    write(outfd, &hdr1, sizeof(Elf32_Ehdr));

    // Start sections at 0x174
    off_t curr_offset = 0x174;
    
    // Process sections
    for (int i = 0; i < hdr1.e_shnum; i++) {
        if (hdr1.e_shstrndx >= hdr1.e_shnum) continue;
        
        Elf32_Shdr new_section = sections1[i];
        new_section.sh_offset = curr_offset;

        // Validate string table access
        if (sections1[hdr1.e_shstrndx].sh_offset + new_section.sh_name >= s->size1) continue;
        const char* name = (char*)(s->map1 + sections1[hdr1.e_shstrndx].sh_offset + new_section.sh_name);

        if (new_section.sh_type == SHT_PROGBITS) {
            // Validate section data access
            if (sections1[i].sh_offset + sections1[i].sh_size <= s->size1) {
                write(outfd, s->map1 + sections1[i].sh_offset, sections1[i].sh_size);

                // Find and merge matching section from second file
                for (int j = 0; j < hdr2->e_shnum; j++) {
                    if (hdr2->e_shstrndx >= hdr2->e_shnum) continue;
                    if (sections2[hdr2->e_shstrndx].sh_offset + sections2[j].sh_name >= s->size2) continue;

                    const char* name2 = (char*)(s->map2 + sections2[hdr2->e_shstrndx].sh_offset + sections2[j].sh_name);
                    if (strcmp(name, name2) == 0) {
                        if (sections2[j].sh_offset + sections2[j].sh_size <= s->size2) {
                            write(outfd, s->map2 + sections2[j].sh_offset, sections2[j].sh_size);
                            new_section.sh_size += sections2[j].sh_size;
                        }
                        break;
                    }
                }
            }
        } else if (new_section.sh_type != SHT_NOBITS) {
            if (sections1[i].sh_offset + sections1[i].sh_size <= s->size1) {
                write(outfd, s->map1 + sections1[i].sh_offset, sections1[i].sh_size);
            }
        }

        // Update section header
        lseek(outfd, hdr1.e_shoff + i * sizeof(Elf32_Shdr), SEEK_SET);
        write(outfd, &new_section, sizeof(Elf32_Shdr));
        
        curr_offset += new_section.sh_size;
        if (new_section.sh_addralign > 1) {
            curr_offset = (curr_offset + new_section.sh_addralign - 1) & ~(new_section.sh_addralign - 1);
        }
        lseek(outfd, curr_offset, SEEK_SET);
    }

    close(outfd);
    printf("Merged file created as 'out.ro'\n");
}

void quit(ElfState* s) {
    if (s->fd1 != -1) {
        munmap(s->map1, s->size1);
        close(s->fd1);
    }
    if (s->fd2 != -1) {
        munmap(s->map2, s->size2);
        close(s->fd2);
    }
    printf("Exiting...\n");
    exit(0);
}

int main(int argc, char **argv) {
    while (1) {
        printf("Choose action:\n");
        for (int i = 0; i < sizeof(menu) / sizeof(menu[0]); i++) {
            printf("%d-%s\n", i, menu[i].name);
        }

        int choice;
        scanf("%d", &choice);
        getchar();

        if (choice >= 0 && choice < sizeof(menu) / sizeof(menu[0])) {
            menu[choice].func(&state);
        } else {
            printf("Invalid option\n");
        }
    }
    return 0;
}