#include "stdio.h"
#include "coniferos.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include <stdint.h>

#define MAX_COMMAND_LEN 1024

// This worked!!!!!! I must have pushed things on the stack in the wrong order!!
int main(char *argv[], int argc) 
{
    printf("Welcome echo\n");
    printf("There are %i arguments\n", argc);
    printf("First argument: \n");
    printf(argv[0]);  // wait... argv is coming out to 1.... did I swap argc and argv?
    printf("\n");

    for (;;) {

    }
}