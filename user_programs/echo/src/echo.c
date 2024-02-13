#include "stdio.h"
#include "coniferos.h"
#include "string.h"
#include "stdlib.h"
#include "memory.h"
#include <stdint.h>

#define MAX_COMMAND_LEN 1024

int main(int argc, char *argv[]) 
{
    for (int i = 0; i < argc; i++) {
        printf("\n");
        printf(argv[i]);
    }
    printf("\n");
}