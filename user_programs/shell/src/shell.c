#include "shell.h"
#include "stdio.h"
#include "coniferos.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"

#define MAX_COMMAND_LEN 1024

int main(int argc, char **argv) 
{
    printf("ConiferOS v0.0.1\n");
    for (;;) {
        printf("> ");
        char path_buf[1024];
        const char *path_prefix = "0:/";                // Hardcoding the path prefix
        int len_path_prefix = strlen(path_prefix);

        strncpy(path_buf, "0:/", len_path_prefix);
        coniferos_terminal_readline(path_buf + len_path_prefix, sizeof(path_buf) - len_path_prefix);

        coniferos_exec(path_buf);
    }
    return 0;
}

/* TODO [RyanStan 11-5-23]
 * Finish logic to parse arguments.
 * Since I'm passing arguments as strings on the stack, I don't need the command_token
 * structure anymore.
 */
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