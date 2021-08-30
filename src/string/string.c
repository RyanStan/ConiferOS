#include "string.h"

int strlen(const char *ptr)
{
        int i = 0;
        while(*ptr != 0) {
                i++;
                ptr++;
        }

        return i;
}

int ctoi(char c) 
{
        if (is_digit(c))
                return c - 48;
        else
                return -1;
}

bool is_digit(char c)
{
        return (int)c > 47 && (int)c < 58;
}