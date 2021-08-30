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