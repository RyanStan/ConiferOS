#include "string.h"

char tolower(char c1)
{
        if (c1 >= 65 && c1 <= 90)
                c1 += 32;

        return c1;
}

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


int strnlen_terminator(const char *s, size_t maxlen, char terminator)
{
     int i = 0;
     for (i = 0; i < maxlen; i++) {
             if (s[i] == '\0' || s[i] == terminator)
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

int strncmp(const char *s1, const char *s2, size_t n)
{
        unsigned char u1, u2;

        while (n-- > 0) {
                u1 = (unsigned char)*s1;
                u2 = (unsigned char)*s2;
                if (u1 != u2)
                        return u1 - u2;
                if (u1 == '\0')
                        return 0;

                s1++;
                s2++;
        }

        return 0;
}

/* Compare two strings up to a given length, ignoring case */
int strnicmp(const char *s1, const char *s2, size_t n)
{
        unsigned char u1, u2;

        while (n-- > 0) {
                u1 = (unsigned char)*s1;
                u2 = (unsigned char)*s2;
                if (u1 != u2 && tolower(u1) != tolower(u2))
                        return tolower(u1) - tolower(u2);
                if (u1 == '\0')
                        return 0;

                s1++;
                s2++;
        }

        return 0;
}

char *strncpy(char* dest, const char* src, size_t n)
{
        int i = 0;
        for (i = 0; i < n; i++) {
                if (src[i] == 0x00)
                        break;

                dest[i] = src[i];
        }

        dest[i] = 0x00;
        return dest;
}
