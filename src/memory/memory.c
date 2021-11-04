#include "memory.h"

void *memset(void *s, int c, size_t n)
{
	char *byte_ptr = (char *)s;
	for (int i = 0; i < n; i++) {
		byte_ptr[i] = (char)c;			// this type cast is fine since the ascii table only goes up to 127
	}
	return s;
}

int memcmp(void *s1, void *s2, size_t n)
{
	char *c1 = s1;
	char *c2 = s2;

	while (n-- > 0) {

		if (*c1 != *c2) {
			return *c1 < *c2 ? -1 : 1;
		}
		c1++;
		c2++;
	}

	return 0;
}

void *memcpy(void *dest, void *src, size_t n)
{
	char *d = dest;
	char *s = src;
	while (n--) {
		*d++ = *s++;
	}
	return dest;
}