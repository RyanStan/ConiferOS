#include "fs/fat/fat16.h"
#include "status.h"
#include "string/string.h"
#include "disk/disk_stream.h"
#include <stdint.h>

#define FAT16_SIGNATURE         0x29
#define FAT16_FAT_ENTRY_SIZE    0x02
#define FAT16_BAD_SECTOR        0xFF7
#define FAT16_UNUSED            0x00

/* FAT directory entry attribute byte bitmasks */
#define FAT_FILE_READ_ONLY      0x01                    // If this bit is set, the operating system will not allow a file to be opened for modification. 
#define FAT_FILE_HIDDEN         0x02                    // Hides files or directories from normal directory views. 
#define FAT_FILE_SYSTEM         0x04                    // Indicates that the file belongs to the system and must not be physically moved because there may be references into the file using absolute addressing (bypassing the fs)
#define FAT_FILE_VOLUME_LABEL   0x08                    // Indicates an optional directory volume label, normally only residing in a volume's root directory
#define FAT_FILE_SUBDIRECTORY   0x10                    // Indicates that the cluster-chain associated with this entry gets intepreted as a subdirectory instead of as a file
#define FAT_FILE_ARCHIVED       0x20                    // Typically set by the OS as soon as the file is created or modified to mark the file as "dirty" and reset by backup software once the file has been backed up to indicate "pure" state
#define FAT_FILE_DEVICE         0x40                    // Internally set for character device names found in filespecs, never found on disk, must not be changed by disk tools
#define FAT_FILE_RESERVED       0x80                    // Reserved, must not be changed by disk tools

/* Will be used to quickly identify whether a directory entry represents a directory or a file.
 * This is used for our own internal data structure and is not written to disk.
 */
enum fat_dir_entry_type {
        FAT_ITEM_TYPE_DIRECTORY,
        FAT_ITEM_TYPE_FILE
};

/* Extended BIOS Parameter block */
struct fat_header_extended {
        uint8_t drive_number;
        uint8_t win_nt_bit;
        uint8_t extended_boot_signature;
        uint32_t volume_id;
        uint8_t volume_id_string[11];                   // Partition Volume Label, padded with blanks
        uint8_t system_id_string[8];                    // File system type, padded with blanks
} __attribute__((packed));

struct fat_header_primary {
        uint8_t short_jmp_ins[3];                       // Instructions to jump to the bootstrap code
        uint8_t oem_id[8];                              // Name of the formatting OS
        /* BIOS Parameter Block starts here */
        uint16_t bytes_per_sector;                      // This value is the number of bytes in each physical sector. The allowed values are: 512, 1024, 2048 or 4096 bytes
        uint8_t sectors_per_cluster;                    // This is the number of sectors per cluster. The allowed values are: 1, 2, 4, 8, 16, 32 or 128
        uint16_t reserved_sectors;                      // Since the reserved region always contain the boot sector a zero value in this field is not allowed. The usual setting of this value is 1. The value is used to calculate the location for the first sector containing the FAT.
        uint8_t fat_copies;                             // # of file allocation tables on the file system
        uint16_t root_dir_entries;                      // Root directory must occupy entire sectors (decimal 64)
        uint16_t number_of_sectors;                     // This field states the total number of sectors in the volume
        uint8_t media_type;                             // Media Descriptor.  Describes the pysical format of the volume (e.g. 3.5-inc, 2-sided, 36-sector, fixed disk, etc...)
        uint16_t sectors_per_fat;                       // # of sectors occupied by one copy of the FAT
        uint16_t sectors_per_track;                     // This value is used when the volume is on a media which has a geometry - the LBA is broken down into a CHS address.  This field represents the multiple of the max. Head and Sector value used when the volume was formatted
        uint16_t number_of_heads;                       // This value is used when the volume is on a media which has a geometry - the LBA is broken down into a CHS address.   This field represents the Head value used when the volume was formatted
        uint32_t hidden_sectors;                        // When the volume is on a media that is partitioned, this value contains the number of sectors preceeding the first sector of the volume.
        uint32_t sectors_big;                           // This field states the total number of sectors in the volume
} __attribute__((packed));

struct fat_header {
        struct fat_header_primary fat_header_primary;
        struct fat_header_extended fat_header_extended;
};

struct fat_directory_entry {
        uint8_t filename[8];                            // Must be trailing padded.  Allowed characters are alphanumeric only
        uint8_t ext[3];                                 // Same requirements as filename
        uint8_t attribute;                              // Defines a set of flags which can be set for directories, volume name, hiden files, system files, etc.
        uint8_t reserved;                               // Reserved for Windows NT
        uint8_t creation_time_millisecond;              // This field (1 byte) only contains the millisecond stamp in counts of 10 milliseconds. Therefore valid values are between 0 and 199 inclusive. 
        uint16_t creation_time;                         // Bits 15 - 11 = Hours (0-23).  Bits 10 - 5 = Minutes (0-59).  Bits 4 - 0 = Seconds (0-29)
        uint16_t creation_date;                         // Bits 15 - 9 = Years from 1980.  Bits 10 - 5 = Month of year (1-12).  Bits 4 - 0 = Day of month (1-31).
        uint16_t last_access;                           // This 16 bit field contain the date when the entry was last read or written to. In case of writes this field of cause contain the same value as the Last Write Date field. 
        uint16_t high_16_bits_first_cluster;            // High two bytes of first cluster number
        uint16_t last_mod_time;                         // This 16 bit field is set to the time when the last write occured. When the entry is create this field and the Creation Time field should contain the same values.
        uint16_t last_mod_date;                         // This 16 bit field is set to the date when the last write occured. When the entry is create this field and the Creation Date field should contain the same values. 
        uint16_t low_16_bits_first_cluster;             // Low two bytes of first cluster number
        uint32_t filesize;                              // This 32-bit field count the total file size in bytes. For this reason the file system driver must not allow more than 4 Gb to be allocated to a file. For other entries than files then file size field should be set to 0.
} __attribute__((packed));

/* fat_directory corresponds to a directory cluster and represents a directory in our filesystem
 * It doesn't directly correspond to an on-disk data structure.  This struct just makes it easier for us to manage things internally
 */
struct fat_directory {
        struct fat_directory_entry *entry;              // Points to the first directory entry in the directory that this struct represents
        int total;                                      // Total number of entries in the directory
        int sector;                                     // The first disk sector where this directory cluster is located
        int end_sector;                                 // The last disk sector that contains this directory cluster
};

/* This structure is meant to make dealing with directory entries simpler.  In the case where the entry we're working with
 * represents a directory, we can just access a fat_directory structure instead of the fat_directory_entry.
 * Similar to fat_directory, this structure does not directly correspond to on-disk data.
 */
struct fat_easy_directory_entry {
        union {
                struct fat_directory_entry *entry;      // If the entry represents a file, we still want to access the fat_directory_entry structure
                struct fat_directory *directory;        // If the entry represents a directory, we can utilize our fat_directory structure instead of fat_directory_entry
        };

        enum fat_dir_entry_type type;
};

/* Represents an open file */
struct fat_item_descriptor {
        struct fat_easy_directory_entry *item;          
        uint32_t pos;                                   // Current file offset
};

/* Private data for internal use by FAT filesystem 
 * (This data structure is just to help make things easier)
 */
struct fat_private {
        struct fat_header header;
        struct fat_directory root_directory;

        struct disk_stream *cluster_read_stream;        // Used to stream data clusters
        struct disk_stream *fat_read_stream;            // Used to stream the file allocation table (FAT)
        struct disk_stream *directory_stream;           // "Used to stream the directory" TODO: what does this mean??? to stream a data cluster containing a directory table??
};

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

