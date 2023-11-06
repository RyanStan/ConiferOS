#ifndef SHELL_H
#define SHELL_H

#define MAX_CMMD_ARG_LEN 512

struct command_token {
    char token[MAX_CMMD_ARG_LEN];
    struct command_token *next;
};

// Split a command string into a linked list of command tokens
struct command_token *parse_command(const char *command, int command_len);

#endif