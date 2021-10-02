#ifndef FAT16_H
#define FAT16_H

#include "fs/file.h"

/* Initializes the fat16 filesystem and returns a pointer to it's implementation */
struct filesystem *fat16_init();

#endif 