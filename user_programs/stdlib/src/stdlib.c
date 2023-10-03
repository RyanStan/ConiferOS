#include "stdlib.h"
#include "coniferos.h"
#include <stdbool.h>

// Reverse a string
static int reverse_str(char *str, int len) 
{
    int l = 0;
    int r = len - 1;

    while (l < r) {
        char tmp = str[l];
        str[l++] = str[r];
        str[r--] = tmp;
    }

    return 0;
}

char* itoa(int num, char* buffer)
{
    bool negative = false;
    const int BASE = 10;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\n';
        return buffer;
    }

    if (num < 0) {
        negative = true;
        num = -num;
    }

    int i = 0;
    while (num != 0) {
        int remainder = num % BASE;
        buffer[i++] = '0' + remainder;
        num /= BASE;
    }

    if (negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    reverse_str(buffer, i);
    return buffer;
}

void *malloc(size_t size) {
    return coniferos_malloc(size);
}

void free(void *ptr) {
    coniferos_free(ptr);
}