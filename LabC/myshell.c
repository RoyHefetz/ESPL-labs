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
#define HISTLEN 10

int debug_mode = 0; //Debug mode flag

// History entry structure
typedef struct history_entry {
    char* command;
    struct history_entry* next;
    struct history_entry* prev;
} history_entry;

// History list structure
typedef struct {
    history_entry* head;
    history_entry* tail;
    int size;
} history_list;

typedef struct process {
    cmdLine* cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;

process* process_list = NULL;
history_list* hist_list = NULL;

history_list* init_history() {
    history_list* list = malloc(sizeof(history_list));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void add_to_history(history_list* hist, const char* cmd) {
    // Don't add empty commands or history commands
    if (strlen(cmd) == 0 || strcmp(cmd, "history") == 0 || 
        strcmp(cmd, "!!") == 0 || cmd[0] == '!') {
        return;
    }

    history_entry* entry = malloc(sizeof(history_entry));
    entry->command = strdup(cmd);
    entry->next = NULL;
    entry->prev = NULL;

    if (hist->size == 0) {
        hist->head = entry;
        hist->tail = entry;
    } else {
        entry->prev = hist->tail;
        hist->tail->next = entry;
        hist->tail = entry;
    }

    hist->size++;

    // Remove oldest entry if list is full
    if (hist->size > HISTLEN) {
        history_entry* old_head = hist->head;
        hist->head = hist->head->next;
        hist->head->prev = NULL;
        free(old_head->command);
        free(old_head);
        hist->size--;
    }
}

void print_history(history_list* hist) {
    if (!hist || hist->size == 0) {
        printf("No commands in history\n");
        return;
    }

    history_entry* current = hist->head;
    int index = 1;
    while (current != NULL) {
        printf("%d %s\n", index++, current->command);
        current = current->next;
    }
}

char* get_command_by_index(history_list* hist, int index) {
    if (!hist || index < 1 || index > hist->size) {
        return NULL;
    }

    history_entry* current = hist->head;
    int current_index = 1;
    while (current != NULL && current_index < index) {
        current = current->next;
        current_index++;
    }

    return current ? current->command : NULL;
}

void free_history(history_list* hist) {
    if (!hist) return;

    history_entry* current = hist->head;
    while (current != NULL) {
        history_entry* next = current->next;
        free(current->command);
        free(current);
        current = next;
    }
    free(hist);
}

int handle_history_command(char* input) {
    if (strcmp(input, "history") == 0) {
        print_history(hist_list);
        return 1;
    }
    
    if (strcmp(input, "!!") == 0) {
        if (hist_list->size == 0) {
            printf("No commands in history\n");
            return 1;
        }
        strcpy(input, hist_list->tail->command);
        return 0;
    }
    
    if (input[0] == '!') {
        int index = atoi(input + 1);
        char* cmd = get_command_by_index(hist_list, index);
        if (cmd == NULL) {
            printf("Invalid history index\n");
            return 1;
        }
        if (strncmp(cmd, "stop ", 5) == 0 || 
            strncmp(cmd, "wake ", 5) == 0 || 
            strncmp(cmd, "term ", 5) == 0) {
            printf("Warning: Executing historical process management command\n");
        }
        strcpy(input, cmd);
        return 0;
    }
    
    return 0;
}

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
            return;
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
    updateProcessList(process_list);
    
    process* curr = *process_list;
    process* prev = NULL;
    
    printf("PID\tCommand\t\tSTATUS\n");
    
    int anyProcesses = 0;
    while (curr != NULL) {
        anyProcesses = 1;
        const char* status_str;
        switch (curr->status) {
            case TERMINATED: status_str = "Terminated"; break;
            case RUNNING: status_str = "Running"; break;
            case SUSPENDED: status_str = "Suspended"; break;
            default: status_str = "Unknown"; break;
        }
        
        printf("%-8d%-16s%s\n", curr->pid, curr->cmd->arguments[0], status_str);
        
        // Remove terminated processes immediately after showing them
        if (curr->status == TERMINATED) {
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
    
    if (!anyProcesses) {
        printf("No active processes\n");
    }
}

void freeProcessList(process* process_list) {
    process* curr = process_list;
    while (curr != NULL) {
        process* next = curr->next;
        if (curr->cmd != NULL) {
            freeCmdLines(curr->cmd);
            curr->cmd = NULL;
        }
        free(curr);
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

    // Initialize history
    hist_list = init_history();
    if (hist_list == NULL) {
        fprintf(stderr, "Failed to initialize history\n");
        exit(EXIT_FAILURE);
    }

    // Check for debug flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = 1;
        }
    }

    while (1) {
        // Display prompt with current working directory
        if (getcwd(cwd, PATH_MAX) != NULL) {
            printf("%s> ", cwd);
        } else {
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

        if (handle_history_command(input)) {
            continue;
        }

        add_to_history(hist_list, input);

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
            } else if (chdir(cmdL->arguments[1]) == -1) {
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

        // Handle pipeline
        if (cmdL->next != NULL) {
            execute_pipeline(cmdL, cmdL->next);
            freeCmdLines(cmdL);
            continue;
        }

        // Execute external commands
        execute(cmdL);
        freeCmdLines(cmdL);
    }

    // Clean up before exit
    freeProcessList(process_list);
    free_history(hist_list);
    return 0;
}