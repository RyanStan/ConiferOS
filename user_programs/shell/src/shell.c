#include "shell.h"
#include "stdio.h"
#include "coniferos.h"
#include "string.h"

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