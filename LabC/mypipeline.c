#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t child1, child2;

    // Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    child1 = fork();

    if (child1 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {  // First child (ls -l)
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        close(STDOUT_FILENO);
        dup(pipefd[1]);
        close(pipefd[1]);
        close(pipefd[0]);
        
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args);
        perror("execvp ls failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);
    fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");
    close(pipefd[1]);

    fprintf(stderr, "(parent_process>forking...)\n");
    child2 = fork();

    if (child2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {  // Second child (tail -n 2)
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
        close(STDIN_FILENO);
        dup(pipefd[0]);
        close(pipefd[0]);
        
        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
        char *args[] = {"tail", "-n", "2", NULL};
        execvp(args[0], args);
        perror("execvp tail failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);
    fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
    close(pipefd[0]);

    fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting...)\n");
    return 0;
}
