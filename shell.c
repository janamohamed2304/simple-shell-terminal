#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

void parent_main();
void on_child_exit(int sig);
void setup_environment();
void shell();
void parse_input(char *input, char **command, char **args, int *background);
int evaluate_expression(char *command, char **args);
void execute_shell_builtin(char *command, char **args);
void execute_command(char *command, char **args, int background);

int main() {
    parent_main();
    return 0;
}

void parent_main() {
    signal(SIGCHLD, on_child_exit); 
    setup_environment();
    shell();
}

void on_child_exit(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { 
        FILE *log_file = fopen("/home/jana/shell/shell1_log.txt", "a");
        if (log_file != NULL) {
            fprintf(log_file, "Child process %d terminated\n", pid);
            fclose(log_file);
        }
    }
}

void setup_environment() {
    chdir("/");
}

void shell() {
    char input[1024];
    char *command = NULL;
    char *args[64] = {NULL}; 
    int background = 0;
    int exit_shell = 0;
    
    do {
        
        printf("shell:~$ "); 
        fflush(stdout);
        

        scanf("%1023[^\n]", input);  
        input[sizeof(input) - 1] = '\0';  
        getchar();  

        parse_input(input, &command, args, &background);
        
        if (command == NULL || *command == '\0') {
            continue;
        }
        
        int cmd_type = evaluate_expression(command, args);
        
        switch (cmd_type) {
            case 1: 
                execute_shell_builtin(command, args);
                break;
            case 2: 
                execute_command(command, args, background);
                break;
            case 0:
                exit_shell = 1;
                break;
            default: 
                printf("Command not found: %s\n", command);
                break;
        }
        
        if (command != NULL) {
            free(command);
            command = NULL;
        }
        
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
            args[i] = NULL;
        }
        
    } while (!exit_shell);
}

void parse_input(char *input, char **command, char **args, int *background) {
    char *token;
    int i = 0;
    
    token = strtok(input, " \t");
    if (token != NULL) {
        *command = strdup(token);
        args[0] = strdup(token); 
        i++;
        while ((token = strtok(NULL, " \t")) != NULL && i < 63) {
            if (strcmp(token, "&") == 0) { 
                *background = 1; 
                continue;
            }
            args[i] = strdup(token);
            i++;
        }
    }
    args[i] = NULL; 
}

int evaluate_expression(char *command, char **args) {
    if (strcmp(command, "exit") == 0) {
        return 0; 
    } else if (strcmp(command, "cd") == 0 || 
               strcmp(command, "echo") == 0 || 
               strcmp(command, "export") == 0) {
        return 1; 
    } else {
        char path[1024];
        char *path_env = getenv("PATH");
        char *path_copy = strdup(path_env);
        char *dir = strtok(path_copy, ":");
        
        while (dir != NULL) {
            snprintf(path, sizeof(path), "%s/%s", dir, command);
            if (access(path, X_OK) == 0) {
                free(path_copy);
                return 2; 
            }
            dir = strtok(NULL, ":");
        }
        
        free(path_copy);
        return -1;
    }
}

void execute_shell_builtin(char *command, char **args) {
    if (strcmp(command, "cd") == 0) {
        if (args[1] == NULL) {
            if (chdir(getenv("HOME")) != 0) {
                perror("cd");
            }
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
    } else if (strcmp(command, "echo") == 0) {
        for (int i = 1; args[i] != NULL; i++) {
            char *arg = args[i];
            if (arg[0] == '$') {
                char *env_var = getenv(arg + 1);
                if (env_var != NULL) {
                    printf("%s ", env_var);
                } else {
                    printf(" ");
                }
            } else {
                printf("%s ", arg);
            }
        }
        printf("\n");
    } else if (strcmp(command, "export") == 0) {
        if (args[1] != NULL) {
            char *equals = strchr(args[1], '=');
            if (equals != NULL) {
                *equals = '\0'; 
                setenv(args[1], equals + 1, 1);
            } else {
                printf("export: usage: export VAR=value\n");
            }
        } else {
            printf("export: usage: export VAR=value\n");
        }
    }
}

void execute_command(char *command, char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) { 
        if (background) {
            setsid(); 
        }
        execvp(command, args);
        perror("Error executing command");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (!background) {
            int status;
            waitpid(pid, &status, 0); 
        } else {
            printf("Started background process: %d\n", pid);
        }
    } else {
        perror("Fork failed");
    }
}
