#include "fs/fat/fat16.h"
#include "status.h"
#include "string/string.h"

/* TODO: add comment */
void *fat16_open(struct disk *disk, struct path_part *path_part, enum file_mode mode)
{
        return 0;
}

/* fat16_resolve
*
* Returns 0 if disk is formatted to FAT16 or < 0 otherwise
* 
* E.g. if the function/filesystem implementing this function pointer is part of our
* FAT filesystem driver, then it will return 0 if disk is formatted as a FAT filesystem
*/
int fat16_resolve(struct disk *disk)
{
        return -EIO;
}

struct filesystem fat16_fs = {
        .resolve = fat16_resolve,
        .open = fat16_open
};

struct filesystem *fat16_init()
{
        strcpy(fat16_fs.name, "FAT16");
        return &fat16_fs;
}

