#include "stdlib.h"
#include "coniferos.h"

void *malloc(size_t size) {
    return coniferos_malloc(size);
}

void free(void *ptr) {
    
}