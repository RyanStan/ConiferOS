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

/* Copies the string pointed to by src, 
 * including the terminating null byte ('\0'), to the 
 * buffer pointed to by dest.  
 * The strings may not overlap, and the destination string dest 
 * must be large enough to receive the copy.
 */
char *strcpy(char *dest, const char *src);


#endif