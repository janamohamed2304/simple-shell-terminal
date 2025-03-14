#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 64
#define LOG_FILE "/home/jana/shell/shell1_log.txt"

void parent_main();
void on_child_exit(int sig);
void setup_environment();
void shell();
void parse_input(char *input, char **command, char **args, int *background);
void execute_shell_builtin(char *command, char **args);
void execute_command(char *command, char **args, int background);
void cleanup_args(char **args);

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
    (void)sig; // Avoid unused parameter warning
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        FILE *log_file = fopen(LOG_FILE, "a");
        if (log_file) {
            fprintf(log_file, "Child process %d terminated\n", pid);
            fclose(log_file);
        }
    }
}

void setup_environment() {
    chdir(getenv("HOME") ? getenv("HOME") : "/");
}

void shell() {
    char input[MAX_INPUT_SIZE];
    char *command = NULL;
    char *args[MAX_ARG_COUNT] = {NULL};
    int background = 0;

    while (1) {
        printf("shell:~$ ");
        fflush(stdout);

        if (scanf(" %1023[^\n]", input) != 1) {
            getchar();  // Clear buffer if input is empty
            continue;
        }
        getchar();  // Consume newline character

        parse_input(input, &command, args, &background);
        if (!command) continue;

        // Check if the command is a built-in command
        if (strcmp(command, "exit") == 0) {
            free(command);
            cleanup_args(args);
            return;
        } else if (strcmp(command, "cd") == 0 || strcmp(command, "echo") == 0 || strcmp(command, "export") == 0) {
            execute_shell_builtin(command, args);
        } else {
            // If not a built-in command, try executing it as an external command
            execute_command(command, args, background);
        }

        free(command);
        cleanup_args(args);
    }
}

void parse_input(char *input, char **command, char **args, int *background) {
    char *token;
    int index = 0;
    *background = 0;

    token = strtok(input, " \t");
    if (!token) return;

    *command = strdup(token);
    args[index++] = strdup(token);

    while ((token = strtok(NULL, " \t")) && index < MAX_ARG_COUNT - 1) {
        if (strcmp(token, "&") == 0) {
            *background = 1;
            continue;
        }
        args[index++] = strdup(token);
    }
    args[index] = NULL;
}

void execute_shell_builtin(char *command, char **args) {
    if (strcmp(command, "cd") == 0) {
        const char *dir = args[1] ? args[1] : getenv("HOME");
        if (chdir(dir) != 0) {
            perror("cd");
        }
    } else if (strcmp(command, "echo") == 0) {
        for (int i = 1; args[i]; i++) {
            if (args[i][0] == '$') {
                char *env_var = getenv(args[i] + 1);
                printf("%s ", env_var ? env_var : "");
            } else {
                printf("%s ", args[i]);
            }
        }
        printf("\n");
    } else if (strcmp(command, "export") == 0) {
        if (!args[1] || !strchr(args[1], '=')) {
            fprintf(stderr, "export: usage: export VAR=value\n");
            return;
        }
        char *var = strtok(args[1], "=");
        char *val = strtok(NULL, "=");
        if (var && val) {
            setenv(var, val, 1);
        }
    }
}

void execute_command(char *command, char **args, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        if (background) setsid();
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

void cleanup_args(char **args) {
    for (int i = 0; args[i]; i++) {
        free(args[i]);
        args[i] = NULL;
    }
}
