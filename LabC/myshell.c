#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <signal.h>
#include "LineParser.h"

#define MAX_INPUT_SIZE 2048
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

int debug_mode = 0; //Debug mode flag

typedef struct process {
    cmdLine* cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;

process* process_list = NULL;

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* new_process = malloc(sizeof(process));
    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}

void updateProcessStatus(process* process_list, int pid, int status) {
    while (process_list != NULL) {
        if (process_list->pid == pid) {
            process_list->status = status;
            return;  // Exit after updating
        }
        process_list = process_list->next;
    }
}

void updateProcessList(process **process_list) {
    process* curr = *process_list;
    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG);
        
        if (result > 0) {  // Process state changed
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                curr->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
}

void printProcessList(process** process_list) {
    updateProcessList(process_list);  // Update status of all processes
    
    process* curr = *process_list;
    process* prev = NULL;
    
    printf("PID\tCommand\t\tSTATUS\n");
    
    while (curr != NULL) {
        const char* status_str;
        switch (curr->status) {
            case TERMINATED: status_str = "Terminated"; break;
            case RUNNING: status_str = "Running"; break;
            case SUSPENDED: status_str = "Suspended"; break;
            default: status_str = "Unknown"; break;
        }
        
        printf("%d\t%s\t\t%s\n", curr->pid, curr->cmd->arguments[0], status_str);
        
        if (curr->status == TERMINATED) {
            // Remove terminated process from list
            if (prev == NULL) {
                *process_list = curr->next;
            } else {
                prev->next = curr->next;
            }
            process* to_free = curr;
            curr = curr->next;
            freeCmdLines(to_free->cmd);
            free(to_free);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}


void freeProcessList(process* process_list) {
    process* curr = process_list;
    while (curr != NULL) {
        process* next = curr->next;

        if (curr->cmd != NULL) {
            freeCmdLines(curr->cmd);  // Free command line structure
            curr->cmd = NULL;         // Avoid double-free
        }

        free(curr);  // Free process structure
        curr = next;
    }
}


void execute_pipeline(cmdLine *cmd1, cmdLine *cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (cmd1->outputRedirect) {
        fprintf(stderr, "Error: Cannot redirect output in the middle of a pipeline\n");
        return;
    }
    if (cmd2->inputRedirect) {
        fprintf(stderr, "Error: Cannot redirect input at the end of a pipeline\n");
        return;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return;
    }

    pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid1 == 0) { // First child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        if (cmd1->inputRedirect) {
            if (freopen(cmd1->inputRedirect, "r", stdin) == NULL) {
                perror("input redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        
        if (debug_mode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s\n", cmd1->arguments[0]);
        }
        
        execvp(cmd1->arguments[0], cmd1->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid2 == 0) { // Second child
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        if (cmd2->outputRedirect) {
            if (freopen(cmd2->outputRedirect, "w", stdout) == NULL) {
                perror("output redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        
        if (debug_mode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s\n", cmd2->arguments[0]);
        }
        
        execvp(cmd2->arguments[0], cmd2->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if (cmd2->blocking) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}

void execute(cmdLine *pCmdLine) {
    // Don't track shell commands
    if (strcmp(pCmdLine->arguments[0], "procs") == 0 ||
        strcmp(pCmdLine->arguments[0], "cd") == 0 ||
        strcmp(pCmdLine->arguments[0], "quit") == 0) {
        return;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) { // Child process
        if (pCmdLine->inputRedirect) {
            if (freopen(pCmdLine->inputRedirect, "r", stdin) == NULL) {
                perror("input redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        
        if (pCmdLine->outputRedirect) {
            if (freopen(pCmdLine->outputRedirect, "w", stdout) == NULL) {
                perror("output redirection failed");
                _exit(EXIT_FAILURE);
            }
        }

        if (debug_mode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        }
        
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    } else {
        // Create a deep copy for the process list
        cmdLine* cmd_copy = parseCmdLines(pCmdLine->arguments[0]);
        for (int i = 1; i < pCmdLine->argCount; i++) {
            replaceCmdArg(cmd_copy, i, pCmdLine->arguments[i]);
        }
        cmd_copy->blocking = pCmdLine->blocking;
        
        addProcess(&process_list, cmd_copy, pid);
        
        if (pCmdLine->blocking) {
            waitpid(pid, NULL, 0);
            updateProcessStatus(process_list, pid, TERMINATED);
        }
    }
}

void handle_signal(cmdLine *pCmdLine) {
    if (pCmdLine->argCount != 2) {
        fprintf(stderr, "Usage: %s <process id>\n", pCmdLine->arguments[0]);
        return;
    }

    pid_t pid = atoi(pCmdLine->arguments[1]);
    
    if (strcmp(pCmdLine->arguments[0], "stop") == 0) {
        if (kill(pid, SIGSTOP) == -1) {
            perror("stop failed");
        } else {
            updateProcessStatus(process_list, pid, SUSPENDED);
        }
    } 
    else if (strcmp(pCmdLine->arguments[0], "wake") == 0) {
        if (kill(pid, SIGCONT) == -1) {
            perror("wake failed");
        } else {
            updateProcessStatus(process_list, pid, RUNNING);
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "term") == 0) {
        if (kill(pid, SIGINT) == -1) {
            perror("term failed");
        } else {
            updateProcessStatus(process_list, pid, TERMINATED);
        }
    }
}


int main(int argc, char *argv[]) {
    char cwd[PATH_MAX];
    char input[MAX_INPUT_SIZE];
    cmdLine *cmdL;

    // Check for debug flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = 1;
        }
    }

    while (1) {
        //Display prompt with current working directory
        if (getcwd(cwd, PATH_MAX) != NULL) {
            printf("%s> ", cwd);
        } 
        else {
            perror("getcwd failed");
            exit(EXIT_FAILURE);
        }
        
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0; //Remove newline char
        
        if (strlen(input) == 0) { //Skip empty lines
            continue;
        }

        cmdL = parseCmdLines(input);
        if (cmdL == NULL) {
            continue;
        }

        // Handle quit command
        if (strcmp(cmdL->arguments[0], "quit") == 0) {
            freeCmdLines(cmdL);
            break;
        }

        if (strcmp(cmdL->arguments[0], "cd") == 0) {
            if (cmdL->argCount < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } 
            else if (chdir(cmdL->arguments[1]) == -1) {
                perror("cd failed");
            }
            freeCmdLines(cmdL);
            continue;
        }

        if (strcmp(cmdL->arguments[0], "stop") == 0 ||
            strcmp(cmdL->arguments[0], "wake") == 0 ||
            strcmp(cmdL->arguments[0], "term") == 0) {
            handle_signal(cmdL);
            freeCmdLines(cmdL);
            continue;
        }

        if (strcmp(cmdL->arguments[0], "procs") == 0) {
            printProcessList(&process_list);
            freeCmdLines(cmdL);
            continue;
        }

        // Execute external commands
        execute(cmdL);
        freeCmdLines(cmdL);
    }
    
    return 0;
}


