#include "memory.h"

void *memset(void *s, int c, size_t n)
{
	char *byte_ptr = (char *)s;
	for (int i = 0; i < n; i++) {
		byte_ptr[i] = (char)c;			// note: this type cast is fine since the ascii table only goes up to 127
	}
	return s;
}

