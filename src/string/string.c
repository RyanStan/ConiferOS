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

int strnlen(const char *s, size_t maxlen)
{
        int i;
        for (i = 0; i < maxlen; i++) {
                if (s[i] == 0)
                        break;
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

char *strcpy(char *dest, const char *src)
{
        char *tmp = dest;
        while (*src != 0) {
                *tmp = *src;
                src++;
                tmp++;
        }
        *tmp = 0;
        return dest;
}