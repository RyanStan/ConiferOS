#include "shell.h"
#include "stdio.h"
#include "coniferos.h"

int main(int argc, char **argv) 
{
    printf("ConiferOS v0.0.1\n");
    for (;;) {
        printf("> ");
        char buf[1024];
        coniferos_terminal_readline(buf, sizeof(buf));
        printf("\n");
    }
    return 0;
}