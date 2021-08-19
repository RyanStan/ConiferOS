#include "io/io.h"
#include "disk.h"
#include "memory/memory.h"
#include "status.h"

#define BUSY_BIT                0x0008                          // If the busy bit is set, the disk drive still has control of the command block
#define ATA_COMMAND_IO_PORT     0x01F7
#define ATA_DATA_PORT           0x01F0
#define ATA_READ_SECTORS        0x0020

struct disk disk;

/* Read sectors at lba 
 * lba - the logical block address to read from
 * total - the number of sectors to read
 * buf - the buffer to store read data
 * 
 * I'm still somewhat unsure as to where the specification with these ports is defined.
 * I'm just using OSDev as a resource for these now.
 * 
 * TODO: pull out the ports into macros
 */
int disk_read_sector(int lba, int total, void *buf)
{
        outb(0x1F6, (lba >> 24) | 0xE0);                        // Select the master drive                       
        outb(0x1F2, total);                                     // Set sectorcount
        outb(0x1F3, (unsigned char)(lba & 0xFF));               // Set LBAlo
        outb(0x1F5, (unsigned char)(lba >> 16));                // Set LBAhi
        outb(ATA_COMMAND_IO_PORT, ATA_READ_SECTORS);            // 0x20 is read sector(s) command
        
        /* Read two bytes at a time */
        unsigned short *ptr = (unsigned short *)buf;
        for (int i = 0; i < total; i++) {

                /* Wait for the buffer to be ready */
                char c = insb(ATA_COMMAND_IO_PORT);             // Get the status register contents
                while(!(c & BUSY_BIT)) {        
                        c = insb(0x1F7);
                }

                /* Copy from the hard disk to memory */
                for (int j = 0; j < DISK_SECTOR_SIZE / 2; j++) {
                        *ptr = insw(ATA_DATA_PORT);
                        ptr++;
                }

        }


        return 0;
}

/* Doesn't do actual search yet.  Just here for the future when we have more disks than the physical hard drive */
void disk_search_and_init()  
{
        memset(&disk, 0, sizeof(struct disk));
        disk.type = REAL;
        disk.sector_size = DISK_SECTOR_SIZE;
}

/* For now, since we only have one disk, the implementation is very basic */
struct disk *disk_get(int index)
{
        if (index != 0)
                return 0;

        return &disk;
}

int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buf)
{
        if (idisk != &disk)
                return -EIO;                            // disk wasn't initialized yet

        return disk_read_sector(lba, total, buf);       // eventually, disk_read_sector will take in a base port # which it acquires from disk
}