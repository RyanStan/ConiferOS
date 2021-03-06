#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

#define DISK_SECTOR_SIZE        512

enum disk_type {
        REAL,                           // represents real physical hard drive
};

struct disk {
        enum disk_type type;
        int id;
        int sector_size;
        struct filesystem *filesystem;  // This is the filesystem that is bound to the disk
        void *fs_private;               // The private data of the filesystem that is bound to this disk
};


/* disk_search_and_init
 * Searches for disks and initializes them.
 * Must be called after filesystems are initialized (fs_init())
 */
void disk_search_and_init();

/* 
 * disk_get
 * Return the disk associated with index 
 *
 * prereq - called disk_search_and_init()
 */
struct disk *disk_get(int index);

/*
 * disk_read_block
 * 
 * idisk - the disk to read from
 * lba - the logical block address (disk sector) to start the read at 
 * total - the total number of sectors to read
 * buf - output buffer to store read data
 * 
 */
int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buf);


#endif