#include "fs/fat/fat16.h"
#include "status.h"
#include "string/string.h"
#include "disk/disk_stream.h"
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
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
        uint16_t root_dir_entries;                      // This value contain the number of possible (max) entries in the root directory. Its recommended that the number of entries is an even multiple of the BytesPerSector values. The recommended value for FAT16 volumes is 512 entries (compatibility reasons).
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
} __attribute__((packed));

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
        uint32_t pos;                                   // Current stream offset into file
};

/* Private data for internal use by FAT filesystem 
 * (This data structure is just to help make things easier)
 */
struct fat_private {
        struct fat_header header;
        struct fat_directory root_directory;

        /* Three separate stream structures but all point to the same disk 
         * that is associated with this fat filesystem.
         */
        struct disk_stream *cluster_read_stream;                        // Used to stream file data from data clusters
        struct disk_stream *fat_read_stream;                            // Used to stream the file allocation table (FAT)
        struct disk_stream *directory_stream;                           // Used to stream directory clusters
};

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path_part, enum file_mode mode);

struct filesystem fat16_fs = {
        .resolve = fat16_resolve,
        .open = fat16_open
};

struct filesystem *fat16_init()
{
        strcpy(fat16_fs.name, "FAT16");
        return &fat16_fs;
}

/* TODO: add comment */
void *fat16_open(struct disk *disk, struct path_part *path_part, enum file_mode mode)
{
        return 0;
}

/* Initializes the filesystem's internal private data structures */
static void fat16_init_private(struct disk *disk, struct fat_private *fat_private)
{
        memset(fat_private, 0, sizeof(struct fat_private));
        fat_private->cluster_read_stream = get_disk_stream(disk->id);
        fat_private->fat_read_stream = get_disk_stream(disk->id);
        fat_private->directory_stream = get_disk_stream(disk->id);
}

/* Returns the absolute position of sector on disk 
 * "Absolute position" refers to the byte offset of sector from the start of the disk
 * Sector is a 0-based index.  I.e. sector 0 starts at byte 0, sector 1 starts at byte 512, etc.
 */
int fat16_sector_to_absolute_pos(struct disk *disk, int sector)
{
        return sector * disk->sector_size;
}

/* Return the number of directory entries in the directory cluster that starts at directory_start_sector 
 * Returns < 0 if an error occurs
 */
int fat16_get_total_items_for_directory(struct disk *disk, uint32_t directory_start_sector)
{
        int rc = 0;

        struct fat_directory_entry entry;
        memset(&entry, 0, sizeof(struct fat_directory_entry));

        int directory_start_pos = fat16_sector_to_absolute_pos(disk, directory_start_sector);
        struct fat_private *fat_private = disk->fs_private;
        struct disk_stream *directory_stream = fat_private->directory_stream;
        if ((rc = disk_stream_seek(directory_stream, directory_start_pos)) < 0) {
                return rc;
        }
        
        /* Read each entry in the directory 
         * TODO: if all the entries have been read from the cluster, we should see if there is another cluster following this one in the cluster chain or if this is the last cluster in the chain 
         * Given we're currently not implementing the above, the max # of directory entries we can have in a directory is 2048.
         * 128 sectors per cluster * 512 bytes per sector / 32 bytes per root directory entry = 2048 directory entries in a single cluster max
         * Actually, the max we can have is 2047 directory entries so that we have a blank entry denoting the end of the directory
         */
        int count = 0;
        while (1) {
                if ((rc = disk_stream_read(directory_stream, &entry, sizeof(struct fat_directory_entry))) < 0) {
                        return rc;
                }

                /* If the first byte of the entry is 0, then there are no more entries in this directory */
                if (entry.filename[0] == 0x00)
                        break;

                /* If the first byte of the entry is 0xE5, then the entry is unused (free) */
                if (entry.filename[0] == 0xE5)
                        continue;

                count++;
        }

        return count;
}

/* TODO: add function comment.
 * Load the root directory into the fat_directory structure out_root_dir
 * Returns 0 on success or < 0 on failure
 */
int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *out_root_dir)
{
        int rc;

        struct fat_header_primary *primary_header = &fat_private->header.fat_header_primary;
        int root_dir_sector = primary_header->fat_copies * primary_header->sectors_per_fat + primary_header->reserved_sectors;
        int root_dir_size = primary_header->root_dir_entries * sizeof(struct fat_directory_entry);      //It looks like I'm assuming that 
        int root_dir_total_sectors = root_dir_size / disk->sector_size;
        if (root_dir_size % disk->sector_size != 0) {
                root_dir_total_sectors++;
        }

        /* Since we are not setting root_dir_entries in the primary header (via our boot.asm file), we need to dynamically resolve this value */
        int root_dir_total_items = fat16_get_total_items_for_directory(disk, root_dir_sector);

        struct fat_directory_entry *root_dir = kzalloc(root_dir_size);
        if (!root_dir)
                return -ENOMEM;

        struct disk_stream *stream = fat_private->directory_stream;
        if ((rc = disk_stream_seek(stream, fat16_sector_to_absolute_pos(disk, root_dir_sector))) < 0)
                return rc;

        if ((rc = disk_stream_read(stream, root_dir, root_dir_size)) < 0)
                return rc;

        out_root_dir->entry = root_dir;
        out_root_dir->total = root_dir_total_items;
        out_root_dir->sector = root_dir_sector;
        out_root_dir->end_sector = root_dir_sector + root_dir_size / disk->sector_size;

        return 0;
        
}

/* fat16_resolve
*
* Reads boot sector of disk and returns 0 if disk is formatted to FAT16 or < 0 otherwise
* TODO: describe possible error return codes here
*/
int fat16_resolve(struct disk *disk)
{
        int rc = 0;

        struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
        fat16_init_private(disk, fat_private);

        disk->fs_private = fat_private;

        struct disk_stream *stream = get_disk_stream(disk->id);
        if (!stream) {
                rc = -ENOMEM;
                goto out;
        }    
        
        if ((rc = disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header))) < 0)
                goto out;
        
        if (fat_private->header.fat_header_extended.extended_boot_signature != 0x29) {
                rc = -EFSNOTUS;
                goto out;
        }
                
        if ((rc = fat16_get_root_directory(disk, fat_private, &fat_private->root_directory)) < 0)
                goto out;

        disk->filesystem = &fat16_fs;
        
out:
        if (stream)
                disk_stream_close(stream);

        if (rc < 0) {
                kfree(fat_private);
                disk->fs_private = 0;
        }

        return rc;
}



