#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <signal.h>
#include "LineParser.h"

#define MAX_INPUT_SIZE 2048

int debug_mode = 0; //Debug mode flag

void execute(cmdLine *pCmdLine) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    
    //Task 3 -----------------------------------------------
    if (pid == 0) { // Child process
        // Handle input redirection
        if (pCmdLine->inputRedirect) {
            if (freopen(pCmdLine->inputRedirect, "r", stdin) == NULL) { //Opens the file in read mode
                perror("input redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        
        // Handle output redirection
        if (pCmdLine->outputRedirect) {
            if (freopen(pCmdLine->outputRedirect, "w", stdout) == NULL) { //Opens the file in write mode
                perror("output redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        //Task 1a - print debug info -------------------------------
        if (debug_mode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        }
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    } 
    //Task 1c --------------------------------
    else if (pCmdLine->blocking) { // Parent process - wait only if there's no '&'  
        waitpid(pid, NULL, 0);
    }
}

//Task 2 --------------------------------------------
void handle_signal(cmdLine *pCmdLine) {
    if (pCmdLine->argCount != 2) { //Check for correct argument count
        fprintf(stderr, "Usage: %s <process id>\n", pCmdLine->arguments[0]);
        return;
    }

    pid_t pid = atoi(pCmdLine->arguments[1]); //Convert string PID to integer
    
    if (strcmp(pCmdLine->arguments[0], "stop") == 0) {
        if (kill(pid, SIGSTOP) == -1) {
            perror("stop failed");
        }
    } 
    else if (strcmp(pCmdLine->arguments[0], "wake") == 0) {
        if (kill(pid, SIGCONT) == -1) {
            perror("wake failed");
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "term") == 0) {
        if (kill(pid, SIGINT) == -1) {
            perror("term failed");
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

        //Task 1b - handle cd command------------------------------
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

        //Task 2 - handle signal commands-----------------
        if (strcmp(cmdL->arguments[0], "stop") == 0 ||
            strcmp(cmdL->arguments[0], "wake") == 0 ||
            strcmp(cmdL->arguments[0], "term") == 0) {
            handle_signal(cmdL);
            freeCmdLines(cmdL);
            continue;
        }

        // Execute external commands
        execute(cmdL);
        freeCmdLines(cmdL);
    }
    
    return 0;
}