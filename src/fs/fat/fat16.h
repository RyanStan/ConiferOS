#ifndef FAT16_H
#define FAT16_H

#include "fs/file.h"

/* Our FAT16 filesystem implementation is only going to support reading from files!  At least, for now. */

/* Initializes the fat16 filesystem and returns a pointer to it's implementation */
struct filesystem *fat16_init();

/* fat16_resolve
* Reads boot sector of disk and returns 0 if disk is formatted to FAT16 or < 0 otherwise
*/
int fat16_resolve(struct disk *disk);

/* Creates a file descriptor corresponding to the file at path on disk 
 * Right now this filesystem implementation only supports file reads, so the only
 * allowed mode is READ.
 * Returns an initialized fat_file_descriptor instance on success, or < 0 on failure
 */
void *fat16_open(struct disk *disk, struct path_part *path, enum file_mode mode);

#endif 