#include "shell.h"
#include "stdio.h"
#include "coniferos.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"

#define MAX_COMMAND_LEN 1024

// Generates a null terminated array of arguments from the command_token list.
// This returned array can be passed to the coniferos_execve function.
// This function will also populate arg_count with the count of arguments.
char **create_argv_array(struct command_token *command, int *arg_count) {
    struct command_token *itr = command;

    if (!itr)
        return 0;

    itr = itr->next;
    int count = 0;
    while (itr != 0) {
        count++;
        itr = itr->next;
    }

    char **argv = (char**)malloc((count + 1) * sizeof(char *));
    if (!argv) {
        printf("shell.c: ERROR: could not allocate memory for argv");
        return 0;
    }

    itr = command->next;
    for (int i = 0; i < count; i++) {
        argv[i] = itr->token;
        itr = itr->next;
    }

    argv[count] = NULL;

    *arg_count = count;
    return argv;
}

struct command_token *parse_command(const char *command, int command_len)
{
    struct command_token *root_command = 0;
    if (command_len > MAX_COMMAND_LEN) {
        return 0;
    }

    char token_buf[1024];
    char *token = strtok(command, ' ', token_buf);
    if (!token) {
        return 0;
    }

    root_command = malloc(sizeof(struct command_token));
    if (!root_command) {
        return 0;
    }

    strncpy(root_command->token, token, sizeof(root_command->token));
    root_command->next = 0;

    
    struct command_token *current = root_command;
    memset(token_buf, 0, sizeof(token_buf));
    token = strtok(NULL, ' ', token_buf);
    while (token != NULL) {
        struct command_token *next_arg = malloc(sizeof(struct command_token));
        if (!next_arg) {
            free(root_command);
            return 0;
        }

        strncpy(next_arg->token, token, sizeof(root_command->token));
        next_arg->next = 0;
        current->next = next_arg;
        current = next_arg;
        memset(token_buf, 0, sizeof(token_buf));
        token = strtok(NULL, ' ', token_buf);
    }

    return root_command;
}

int main(int argc, char **argv) 
{
    printf("ConiferOS shell v0.0.1\n");
    printf("-----------------------\n");
    printf("Try executing 'ECHO.ELF HELLO'\n");
    printf("-----------------------\n\n");

    for (;;) {
        printf("> ");

        char path_buf[MAX_COMMAND_LEN];
        const char *path_prefix = "0:/";                // Hardcoding the path prefix
        int len_path_prefix = strlen(path_prefix);
        strncpy(path_buf, "0:/", len_path_prefix);
        coniferos_terminal_readline(path_buf + len_path_prefix, sizeof(path_buf) - len_path_prefix);

        struct command_token *root = parse_command(path_buf, MAX_COMMAND_LEN);

        int argc = 0;
        const char **argv = create_argv_array(root, &argc);

        coniferos_execve(root->token, argv, argc);
    }
    return 0;
}
