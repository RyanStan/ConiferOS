#include "io/io.h"

#define BUSY_BIT                0x0008                          // If the busy bit is set, the disk drive still has control of the command block
#define ATA_COMMAND_IO_PORT     0x01F7
#define ATA_DATA_PORT           0x01F0
#define ATA_READ_SECTORS        0x0020

#include "disk.h"

/* 
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