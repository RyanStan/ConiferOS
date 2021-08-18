#ifndef DISK_H
#define DISK_H

#define DISK_SECTOR_SIZE        512

/* Read sectors at lba 
 * lba - the logical block address to read from
 * total - the number of sectors to read
 * buf - the buffer to store read data
 */
int disk_read_sector(int lba, int total, void *buf);

#endif