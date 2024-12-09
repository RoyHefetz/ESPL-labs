#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MESSAGE "hello"
#define BUFFER_SIZE 100

int main() {
    int pipefd[2];  //Array to hold read and write file descriptions
    char buffer[BUFFER_SIZE];
    pid_t pid;

    //Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  //Child process
        close(pipefd[0]); //Close unused read end
        
        write(pipefd[1], MESSAGE, strlen(MESSAGE) + 1);
        
        close(pipefd[1]);  // Close write end
        _exit(EXIT_SUCCESS);
    } 
    else {  //Parent process
        close(pipefd[1]);  //Close unused write end
        
        read(pipefd[0], buffer, BUFFER_SIZE);
        printf("Message from child: %s\n", buffer);
        
        close(pipefd[0]);  //Close read end
        wait(NULL);  //Wait for child to finish
    }

    return 0;
}