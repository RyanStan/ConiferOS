#include "stdio.h"
#include "coniferos.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include <stdint.h>

#define MAX_COMMAND_LEN 1024

int main(int argc, char *argv[]) 
{
    printf("\nWelcome to echo\n");
    printf("There are %i arguments. Arguments separated by newline:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf(argv[i]);
        printf("\n------------\n");
    }

    for (;;) {

    }
}