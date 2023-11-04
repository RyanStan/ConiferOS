#ifndef CONIFER_OS_STRING_H
#define CONIFER_OS_STRING_H

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

/* Extract tokens from strings
 *
 * Description from the C standard library man page for strtok:
 *
 * The strtok() function breaks a string into a sequence of zero or
       more nonempty tokens.  On the first call to strtok(), the string
       to be parsed should be specified in str.  In each subsequent call
       that should parse the same string, str must be NULL.

       The delim argument specifies a set of bytes that delimit the
       tokens in the parsed string.  The caller may specify different
       strings in delim in successive calls that parse the same string.

       Each call to strtok() returns a pointer to a null-terminated
       string containing the next token.  This string does not include
       the delimiting byte.  If no more tokens are found, strtok()
       returns NULL.

* There are two differences between the strtok here and the strtok that's defined in the POSIX standards
 * - delim is only a single character.
 * - users are required to pass in a buffer, out_tok, that we can write the result token to.
 * 
 * TODO [RyanStank 11-4-23] Once we have user space malloc, we can remove out_tok and just allocate the returned token
 *                          on the heap.
 */
char *strtok(char *str, const char delim, char *out_tok);

#endif