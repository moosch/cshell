#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#define EXEC_NAME_LIMIT 16 // Limit of executable name size
#define BUILTIN_COUNT 5

const char *builtins[BUILTIN_COUNT] = {"exit", "echo", "type", "pwd", "cd"};

typedef struct {
    char *paths[250]; // "/usr/bin"
    int count;
} Executables;

typedef struct {
  char cmd[100];
  char *args[250]; // Array of args strings.
  int args_count;
} Command;

Command **executables;
int executables_count = 0;
int is_running = 0;
 
void define_executables(const char *path, Executables *execs)
{
    execs->count = 0;
    for (int i = 0; i < strlen(path); ++i) {
        if (path[i] == ':') {
            execs->paths[execs->count] = (char*)malloc(i + 1);
            strncpy(execs->paths[execs->count], path, i);
            execs->paths[execs->count][i] = '\0';
            execs->count++;
            path += i + 1;
            i = 0;
        }
    }
    execs->paths[execs->count] = malloc(strlen(path) + 1);
    strcpy(execs->paths[execs->count], path);
    execs->count++;
}

void parse_input(char *input, Command *cmd) {
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] == '\n' || input[i] == '\0' || input[i] == ' ') {
            cmd->cmd[i] = '\0';
            break;
        }
        cmd->cmd[i] = input[i];
    }

    char *args_str = input + strlen(cmd->cmd) + 1;
    cmd->args_count = 0;

    for (int i = 0; i < strlen(args_str); i++) {
        if (args_str[i] == ' ' || args_str[i] == '\n' || args_str[i] == '\0') {
            cmd->args[cmd->args_count] = malloc(i + 1);
            strncpy(cmd->args[cmd->args_count], args_str, i);
            cmd->args[cmd->args_count][i] = '\0';
            cmd->args_count++;
            args_str += i + 1;
            i = 0;
        }
    }
}

void handle_echo(Command *cmd)
{
    for (int i = 0; i < cmd->args_count; ++i) {
        printf("%s", cmd->args[i]);
        if (i != cmd->args_count - 1)
            printf(" ");
    }
    printf("\n");
}

char* check_executables(char *cmd, Executables *execs)
{
    // Check if file exists in paths.
    for (int i = 0; i < execs->count; ++i) {
        char *path = malloc(strlen(execs->paths[i]) + strlen(cmd) + 2);
        strcpy(path, execs->paths[i]);
        strcat(path, "/");
        strcat(path, cmd);
        if (access(path, F_OK) != -1)
            return path;
    }
    return NULL;
}

void handle_type(char *cmd, Executables *execs)
{
    for (int i = 0; i < BUILTIN_COUNT; ++i) {
        if (strcmp(builtins[i], cmd) == 0) {
            printf("%s is a shell builtin\n", cmd);
            return;
        }
    }

    char *path = check_executables(cmd, execs);
    if (path == NULL)
        printf("%s: not found\n", cmd);
    else
        printf("%s is %s\n", cmd, path);
}

void execute_cmd(Command *cmd, char *path)
{
    char str[500];
    strcpy(str, path);
    strcat(str, " ");

    for (int i = 0; i < cmd->args_count; ++i) {
        strcat(str, cmd->args[i]);
        if (i != cmd->args_count - 1)
            strcat(str, " ");
    }
    system(str);
}

void handle_command(Command *cmd, Executables *execs)
{
    for (int i = 0; i < BUILTIN_COUNT; ++i) {
        if (strcmp(builtins[i], cmd->cmd) == 0) {
            execute_cmd(cmd, cmd->cmd);
            return;
        }
    }

    char *path = check_executables(cmd->cmd, execs);
    if (path == NULL)
        printf("%s: command not found\n", cmd->cmd);
    else
        execute_cmd(cmd, path);
}

void handle_pwd()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)))
        printf("%s\n", cwd);
    else
        perror("getcwd error, too large");
}

void handle_cd(char *dir)
{
    if (!dir) {
        perror("No directory");
        return;
    }

    if (dir[0] == '~') {
        char *home = getenv("HOME");
        char d[250];
        dir++;
        strcpy(d, home);
        strcat(d, dir);
        if (chdir(d) != 0)
            printf("cd: %s: No such file or directory\n", d);
        return;
    }

    if (chdir(dir) != 0)
        printf("cd: %s: No such file or directory\n", dir);
    return;
}

int main() {
    const char *path = getenv("PATH");
    Executables execs;

    // Add executables
    define_executables(path, &execs);

    is_running = 1;

    while (is_running) {
        printf("$ ");
        fflush(stdout);

        // Wait for user input
        char input[100];
        fgets(input, 100, stdin);

        Command cmd;
        parse_input(input, &cmd);

        char path_c[100];

        if (strcmp("exit", cmd.cmd) == 0) {
            if (cmd.args_count == 0)
                return EXIT_SUCCESS;
            else
                return atoi(cmd.args[0]);
        }

        if (strcmp("echo", cmd.cmd) == 0)
            handle_echo(&cmd);
        else if (strcmp("pwd", cmd.cmd) == 0)
            handle_pwd();
        else if (strcmp("type", cmd.cmd) == 0)
            handle_type(cmd.args[0], &execs);
        else if (strcmp("cd", cmd.cmd) == 0)
            handle_cd(cmd.args[0]);
        else
            handle_command(&cmd, &execs);
    }

    return EXIT_SUCCESS;
}

