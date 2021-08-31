#include "memory.h"

void *memset(void *s, int c, size_t n)
{
	char *byte_ptr = (char *)s;
	for (int i = 0; i < n; i++) {
		byte_ptr[i] = (char)c;			// note: this type cast is fine since the ascii table only goes up to 127
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