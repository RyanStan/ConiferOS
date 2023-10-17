#include "stdio.h"
#include "coniferos.h"
#include <stdarg.h>
#include "stdlib.h"

int putchar(int character)
{
    coniferos_putchar((char )character);
    return 0;
}

int printf(const char *fmt, ...)
{
    va_list v1;
    const char *p;
    char* sval;
    int ival;
    char str[24] = {0}; // Buffer for integers. 

    va_start(v1, fmt);
    for (p = fmt; *p; p++)
    {
        if (*p != '%')
        {
            putchar(*p);
            continue;
        }

        p++;
        switch (*p)
        {
        case 'i':
            // TODO [ryanstan 10-16-23] Validate the size of i to make sure it fits the string buffer
            ival = va_arg(v1, int);
            char str[12] = {0};
            itoa(ival, str);
            print(str);
            break;

        case 's':
            sval = va_arg(v1, char *);
            print(sval);
            break;

        default:
            putchar(*p);
            break;
        }
    }
    va_end(v1);
    return 0;
}