#include "stdio.h"
#include "coniferos.h"

int putchar(int character)
{
    coniferos_putchar((char )character);
    return 0;
}