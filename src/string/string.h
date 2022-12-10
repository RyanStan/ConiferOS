#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stddef.h>

/* If c1 is an upper-case letter, this returns the corresponding
 * lower-case letter.  Otherwise, just returns the same char that was passed in.
 */
char tolower(char c1);

/* Returns the length of the null terminated string at ptr */
int strlen(const char *ptr);

/* Returns the size of the string pointed to by s, excluding the null byte, but at most maxlen */
int strnlen(const char *s, size_t maxlen);

/* Returns the size of the string pointed to by s, which must be at most maxlen length.  The string
 * must be terminated with either the null byte, or the terminator byte.  Either byte is excluded from the size.
 */
int strnlen_terminator(const char *s, size_t maxlen, char terminator);

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

/* Compares the first n bytes of two strings, s1 and s2.  Returns an integer 
 * less than, equal to, or greater than 0, if s1 is found, respectively, 
 * to be less than, to match, or be greater than s2
 */
int strncmp(const char *s1, const char *s2, size_t n);

/* Ignoring case, compares the first n bytes of two strings, s1 and s2.  Returns an integer 
 * less than, equal to, or greater than 0, if s1 is found, respectively, 
 * to be less than, to match, or be greater than s2
 */
int strnicmp(const char *s1, const char *s2, size_t n);

/* Copies the string pointed to by src, including the terminating null byte ('\0'), 
 * to the buffer pointed to by dest.  At most n bytes of src are copied.
 * Returns a pointer to the destination string dest.
 */
char *strncpy(char* dest, const char* src, size_t n);

#endif