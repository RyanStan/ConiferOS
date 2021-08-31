#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stddef.h>

/* Returns the length of the null terminated string at ptr */
int strlen(const char *ptr);

/* Returns the size of the string pointed to by s, excluding the null byte, but at most maxlen */
int strnlen(const char *s, size_t maxlen);

/* Converts the character argument c to an integer 
 * Expects c to be a character from 0-9.  Returns -1 if not true
 */
int ctoi(char c);

/* Returns true if the character c is a digit between '0' and '9' */
bool is_digit(char c);


#endif